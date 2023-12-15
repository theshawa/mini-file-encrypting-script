#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>

#define WRITE_CHUNK_SIZE 1024

int valid_path(char* path){
    struct stat statbuf;
    if(stat(path,&statbuf)==-1) return 0;
    return 1;
}

char* read_content(char* path){
    FILE* file = fopen(path,"r");
    if(!file){
        perror("Unable to open src file!\n");
        exit(1);
    }
    fpos_t original;
    if(fgetpos(file,&original)!=0){
        perror("fgetpos() failed!\n");
        exit(1);
    }
    fseek(file,0,SEEK_END);
    long size = ftell(file);
    if(fsetpos(file,&original)!=0){
        perror("fsetpos() failed!\n");
        return NULL;
    }
    char* contents = malloc(size+1);
    char* write_ite = contents;
    size_t bytes_read = 0;
    while(bytes_read<size){
        size_t current_read_bytes =  fread(contents,1,size-bytes_read,file);
        bytes_read += current_read_bytes;
        write_ite += current_read_bytes;
    }
    if(bytes_read!=size){
        free(contents);
        return NULL;
    }
    contents[size] = '\0';
    fclose(file);
    return contents;
}

void write_file(char* path,char* content){
    FILE* file = fopen(path,"w+");
    if(!file){
        perror("Unable to open file to write to it.\n");
        exit(1);
    }
    size_t size = strlen(content);
    char buffer[WRITE_CHUNK_SIZE];
    size_t remaining = size;
    char* write_ite = content;
    while(remaining){
        size_t chunk_size = remaining < WRITE_CHUNK_SIZE ? remaining : WRITE_CHUNK_SIZE;
        memcpy(buffer,write_ite,chunk_size);
        fwrite(buffer,1,chunk_size,file);
        write_ite+=chunk_size;
        remaining-=chunk_size;
    }
    fclose(file);
}

void encrypt_file(char *path,int key){
    FILE* file = fopen(path,"r");
    char* contents = read_content(path);
    if(!contents){
        perror("Unable to read file due to an error\n");
        exit(1);
    }
    char* encrypted = malloc(strlen(contents)+1);
    for(int i=0;contents[i]!='\0';i++) encrypted[i] = contents[i] ^ key;
    encrypted[strlen(contents)] = '\0';
    char* output_filename = malloc(strlen(path)+10+1);
    sprintf(output_filename,"%s.encrypted",path);
    write_file(output_filename,encrypted);
    free(output_filename);
    free(contents);
    free(encrypted);
    printf("%s encrypted.\n",path);
}

void encrypt_src(char *src,int key){
    DIR* dir = opendir(src);
    if(dir){
        struct dirent* entry;
        while((entry=readdir(dir))!=NULL){
            if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0) continue;
            char* csrc = malloc(strlen(src)+1+strlen(entry->d_name)+1);
            sprintf(csrc,"%s/%s",src,entry->d_name);
            encrypt_src(csrc,key);
            free(csrc);
        }
    }else encrypt_file(src,key);
}

int main(int argc, char** args){
    if(argc<3){
        printf("USAGE: %s <SRC_PATH> <KEY>\n", args[0]);
        return 1;
    }

    char* src = args[1];
    char* key_str = args[2];

    if(!valid_path(src)){
        perror("Invalid path provided!\n");
        return 1;
    }
    
    int key = 0;
    for(int i=0;key_str[i]!='\0';i++) key+=key_str[i];
    key = floor(sin(key)*1000);
    if(key%256==0) key++;
    encrypt_src(src, key);
    return 0;
}
