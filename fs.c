#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include "mymalloc.h"

#define BLKSIZE 512
#define ostream stdout
#define istream stdin
#define estream stderr

extern int errno;

struct metadata{
	char name[256];
	int size;
	int block[62];
	int next;
};

struct usedata{
	int use;
	char data[BLKSIZE-sizeof(int)];
};

int VD_fd;
char nullbyte[] = "\0";
char *buf,*BUF,*file_path,*file_name;
int indx , flag=0, LAST_BLK_NO=0, START_DATA_BLK;
struct metadata * MD , * MD_hover;
struct usedata * UD , * UD_hover;
int chunk=BLKSIZE-sizeof(int);

int writeDiskBlock(int fd , int blkno , char * bufptr){
	if(blkno>=LAST_BLK_NO)
		return -2;
	lseek(fd,blkno*BLKSIZE,0);
	if(flag>0){
		int len=  flag + sizeof(int);
		return write(fd,bufptr,len);
	}
	else
		return write(fd,bufptr,BLKSIZE);
}

int readDiskBlock(int fd , int blkno , char * bufptr){
	if(blkno>=LAST_BLK_NO)
		return -2;
	lseek(fd,blkno*BLKSIZE,0);
	if(flag>0){
		int len= flag + sizeof(int);
		return read(fd,bufptr,len);
	}
	return read(fd,bufptr,BLKSIZE);
}

off_t block_loc(char * PATH){
	off_t i=0;
	readDiskBlock(VD_fd,i,BUF);
	MD=(struct metadata *)BUF;
	while(i<START_DATA_BLK){
		if(strcmp(MD->name,PATH)==0)
			return i;
		readDiskBlock(VD_fd,++i,BUF);
		MD=(struct metadata *)BUF;
	}
	return -1;
}

int deleteFileFromDisk(char * PATH){
	off_t loc= block_loc(PATH);
	if(loc==-1){
		return loc;
	}
	else{
		readDiskBlock(VD_fd,loc,BUF);
		int indx=0,temp_size=0;
		char * nullarr=(char *)mymalloc(sizeof(char)*BLKSIZE);
		bzero(nullarr,BLKSIZE);
		MD=(struct metadata *)BUF;
		while(temp_size!=MD->size || MD->next!=-1){
			if(MD->size - temp_size>=chunk){
				writeDiskBlock(VD_fd,MD->block[indx],nullarr);
				indx++;
				temp_size+=chunk;
			}
			else{
				flag=MD->size - temp_size;
				writeDiskBlock(VD_fd,MD->block[indx],nullarr);
				temp_size+=flag;
				flag=0;
				writeDiskBlock(VD_fd,loc,nullarr);
			}
			if(indx>61){
				int next=MD->next;
				writeDiskBlock(VD_fd,loc,nullarr);
				readDiskBlock(VD_fd,next,BUF);
				loc=next;
				temp_size=0;
				MD=(struct metadata*)BUF;
				indx=0;
			}
		}
		myfree(nullarr);
		return -6;
	}
}

int getFileFromDisk(char * PATH){
	off_t loc= block_loc(file_name);
	if(loc==-1){
		return loc;
	}
	else{
		int file_fd=open(PATH, O_WRONLY | O_CREAT,00700);
		readDiskBlock(VD_fd,loc,BUF);
		int temp_size=0;
		indx=0;
		MD=(struct metadata *)BUF;
		while(temp_size != MD->size || MD->next != -1){
			if((MD->size - temp_size)>=chunk){
				int blk_index=MD->block[indx];
				readDiskBlock(VD_fd,blk_index,buf);
				UD=(struct usedata *)buf;
				write(file_fd,UD->data,chunk);
				temp_size=temp_size + chunk;
				indx++;
			}
			else{
				flag=MD->size - temp_size;
				readDiskBlock(VD_fd,MD->block[indx],buf);
				UD=(struct usedata *)buf;
				write(file_fd,UD->data,flag);
				temp_size+=flag;
				flag=0;
			}
			if(indx>61){
				readDiskBlock(VD_fd,MD->next,BUF);
				MD=(struct metadata *)BUF;
				indx=0;
				temp_size=0;
			}
		}
		return -5;
		close(file_fd);
	}
}

int storeFileOntoDisk(char * PATH){
	int file_fd=open(PATH,O_RDONLY);
	if(file_fd==-1){
		return file_fd;
	}
	else{
		int file_len=lseek(file_fd,0,2);
		lseek(file_fd,0,0);
		int indx=0,datablock=START_DATA_BLK,metahover=0,carry=0;
		readDiskBlock(VD_fd,datablock,buf);//reading first block of usable data
		UD=(struct usedata *)buf;
		readDiskBlock(VD_fd,metahover,BUF);//reading first block of metadata
		MD=(struct metadata *)BUF;
		while(MD->size!=0){
			readDiskBlock(VD_fd,++metahover,BUF);
			if(metahover>=START_DATA_BLK)//if metahover surpasses START_DATA_BLK
				return -3;
			MD=(struct metadata*)BUF;
		}
		MD_hover=MD;
		MD_hover->next=-1;
		while(file_len!=0){
			if(carry==0){
				strcpy(MD_hover->name,file_name);
				carry++;
			}
			while(UD->use!=0){
				if((errno=readDiskBlock(VD_fd,++datablock,buf)) == -2)//reading usable datablocks to find empty blocks
					{
						writeDiskBlock(VD_fd,metahover,(char *)MD_hover);
						return errno;
					}
				UD=(struct usedata *)buf;
			}
			UD_hover=UD;
			UD_hover->use=1;
			if(file_len>chunk){
				read(file_fd,UD_hover->data,chunk);
				if((errno= writeDiskBlock(VD_fd,datablock,(char *)UD_hover)) ==-2)
					return errno;
				MD_hover->block[indx]=datablock;
				MD_hover->size+=chunk;
				indx++;
				file_len-=chunk;
			}
			else{
				flag=file_len;
				read(file_fd,UD_hover->data,flag);
				if((errno= writeDiskBlock(VD_fd,datablock,(char *)UD_hover)) ==-2)
					return errno;
				MD_hover->block[indx]=datablock;
				MD_hover->size+=flag;
				file_len-=flag;
				flag=0;
				if((errno=writeDiskBlock(VD_fd,metahover,(char *)MD_hover))==-2)
					return errno;
			}
			if(indx>61){
				indx=0;
				if((errno= writeDiskBlock(VD_fd,metahover,(char *)MD_hover)) ==-2)
					return errno;
				int temp=metahover;
				readDiskBlock(VD_fd,metahover,BUF);
				MD=(struct metadata *)BUF;
				while(MD->size!=0){
					readDiskBlock(VD_fd,++metahover,BUF);
					if(metahover>=START_DATA_BLK)//if metahover surpasses START_DATA_BLK
						return -3;
					MD=(struct metadata *)BUF;
				}
				readDiskBlock(VD_fd,temp,BUF);
				MD_hover=(struct metadata*)BUF;
				MD_hover->next=metahover;
				if((errno= writeDiskBlock(VD_fd,temp,(char *)MD_hover)) ==-2)
					return -2;
				readDiskBlock(VD_fd,metahover,BUF);
				MD_hover=(struct metadata *)BUF;
				MD_hover->next=-1;
			}
		}
		return -4;
	}
	close(file_fd);
}

void Vdls(char * bufptr){
	off_t blkno=0;
	int sum=0;
	while(blkno < START_DATA_BLK){
		readDiskBlock(VD_fd,blkno,buf);
		MD=(struct metadata *)buf;
		if((strlen(MD->name) + 1 + sum)>=(BLKSIZE -1)){
			bufptr[sum]='\0';
			fprintf(ostream,"%s",bufptr);
			bzero(bufptr,BLKSIZE);
			sum=0;
		}
		if((strlen(MD->name) + 1 + sum)<(BLKSIZE-1)){
			if(strlen(MD->name)>0){
				strcat(bufptr,MD->name);
				strcat(bufptr,"\n");
				sum+= strlen(MD->name) + 1;
			}
		}
		blkno++;
	}
	fprintf(ostream,"%s", bufptr);
}

int VdCpto(char * fpath,char *fname){
	strcat(fpath,fname);
	return storeFileOntoDisk(fpath);
}

int VdCpfrom(char * fpath,char *fname){
	strcat(fpath,fname);
	return getFileFromDisk(fpath);
}

int main()
{
	init();
    off_t disksize = 1024*1024*20L - 1L;
    LAST_BLK_NO = (disksize+1)/BLKSIZE;
    START_DATA_BLK=LAST_BLK_NO/2;
    VD_fd = open("disk.teasage", O_RDWR | O_CREAT,00700);
    buf=(char *)mymalloc((sizeof(char)*BLKSIZE));
    BUF=(char *)mymalloc((sizeof(char)*BLKSIZE));
    file_path=(char *)mymalloc(sizeof(char)* BLKSIZE);
    file_name=(char *)mymalloc(sizeof(char)* 256);
    char * fnames;
    if (VD_fd ==-1) //some error
    {
        printf("Error: cannot create the grand disk.\n Errorno: %d\n", errno);
        perror("Open error.");
        exit(1);
    }
    
    //writing  null byte to the disk
    if (lseek(VD_fd, disksize, SEEK_SET) == -1)
        perror("lseek error");

    if (write(VD_fd, nullbyte, 1) != 1)
        perror("write error");

    lseek(VD_fd,0,0);
    while(1){
    	int ch;
    	fscanf(istream, "%d",&ch);
    	switch(ch){
    		case 1:	fscanf(istream, "%s", file_path);
    				fscanf(istream, "%s", file_name);
    			    errno=VdCpto(file_path,file_name);
    			    if(errno==-1){
    			    	printf("Error: File not found. \n Errorno: %d \n",errno);
    			    	perror("Open error");
    			    }
    			    else if(errno==-2 || errno==-3){
    			    	printf("Error: Storage full. \n Errorno: %d \n",errno);
    			    	perror("Read-Write error");
    			    }
    			    else if(errno==-4){
    			    	printf("File stored successfully. \n Errorno: %d \n",errno);
    			    }
    			    break;
    		case 2: fscanf(istream, "%s", file_path);
    				fscanf(istream, "%s", file_name);
    			    errno=VdCpfrom(file_path,file_name);
    			    if(errno==-1){
    			    	printf("Error: File not found. \n Errorno: %d \n",errno);
    			    	perror("Open error");
    			    }
    			    else if(errno==-2){
    			    	printf("Error: Storage full. \n Errorno: %d \n",errno);
    			    	perror("Read error");
    			    }
    			    else if(errno==-5){
    			    	printf("File copied out successfully. \n Errorno: %d \n",errno);
    			    }
    				break;
    		case 3: fscanf(istream, "%s", file_name);
    			    errno=deleteFileFromDisk(file_name);
    			    if(errno==-1){
    			    	printf("Error: File not found. \n Errorno: %d \n",errno);
    			    	perror("Open error");
    			    }
    			    else if(errno==-2 || errno==-3){
    			    	printf("Error: Storage full. \n Errorno: %d \n",errno);
    			    	perror("RW error");
    			    }
    			    else if(errno==-6){
    			    	printf("File deleted.[%s] \n Errorno: %d \n",file_name,errno);
    			    }
    				break;
    		case 4: fnames=(char *)mymalloc(sizeof(char) * BLKSIZE);
    				Vdls(fnames);
    				myfree(fnames);
    				break;
    		default:myfree(buf);
    				myfree(BUF);
    				myfree(file_name);
    				myfree(file_path);
    				close(VD_fd);
    				exit(1);
    	}
    }
    return 0;
}