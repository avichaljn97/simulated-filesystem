
					An overview of the approach, followed to achieve babystep3.
----------------------------------------------------------------------------------------------------------------------------------------------

Size of the disk= 512*N bytes, where N>=2
Size of each block= 512 bytes//BLKSIZE

So, before start approaching towards the process of storing the data(or files) in our virtual disk, I would like to explain from where the
idea of reserving few blocks to store the information of the file came into play. So, the idea or the inspiration came from a typical 
book, a book has few pages reserved in the starting, which in layman language is called index, the index tells about the chapters and
the sub-topics covered under a particular chapter, not just that, the important part or the application of the index page is that it tells
on which page the particular chapter or sub-topic is printed, which makes searching very fast. In the same way the reserved blocks are
structured and the reserved blocks holds the same importance in  virtual disk as the index in any book.

So, how does the reserved block looks like OR how it is structured?
struct metadata{
	char name[256];//to store the name of the file in the first reserved block of each file.
	int size;//to store the size of the data the current reserved is linked to.
	int blocks[62];//storing the block numbers in which the data is stored.
	int next;//if the information is big enough to be stored within 62 blocks, the information of rest of the data is stored in the
		   reserved block no. "next".
		 //next=-1, if there is no further data left. 
}

How can the program during the runtime can identify the which reserved block is free to store file information and which is not?
The answer to this is that where the program finds metadata->size = 0, thats means there is no information stored in that reserved block and is feasible to store information.

How much space the reserved blocks are occuping in the virtual disk and why?
The reserved blocks are occuping half the space of the virtual disk and the reason behind that is, what if the virtual disk is filled with
all files of size 1 byte, in that case each file will require one reserved block, and probably thats the extreme case of storing files in my virtual disk.

How and where the files are stored?
The files are stored after the space occupied by reserved blocks, and to avoid the complications* of whether the data block is free or not I have structured** the data block in a way that assures to give information about whether the block has some data or not.

* What were the things that lead to the occurance of complications?
Since, files of any type(audio, video, text, image etc) are same with respect the system i.e they all are represented in the form of 0's and 1's but with a readable point of view it some type of file may have several bytes which might give wrong information to the program about the availibity of the a block. Eg- an image file may contain a complete block of '\0' or null values, so the program might assume it as a free block and allocte some other data in it, which will distort the data of that image and now two reserved blocks have stored that particular data block number as a part of their informamtion. So to overcome this the datablock has different structure**.

** Structure of datablock
struct usedata{
	int use;//to indicate whether the block is useable or not, 0 for useable and 1 for occupied.
	char data[BLKSIZE-sizeof(int)];//508 bytes to store the data.
};

So, how is a file stored in virtual disk from actual disk?
Firstly, a pointer of type that of reserved block is hovered on all the reserved blocks to check whether the reserved block is free or not, if a reserved block is found free then a usable data block is searched in the same way i.e by hovering a pointer of its type over all the data blocks, if a data block is found free then 508 byte of data is copied in the block from the file in actual disk and the usedata->use is set to 1 and the block number is stored in the reserved block with the size being updated everytime. The name of the file is stored in the first reserved block, if the file is bigger that 508*62 bytes then next reserved block is searched and the current reserved block is written into the virtual disk with the location of its next reserved block stored in it. In this whole process the following errors may occure,
- all the reserved blocks are occupied.
- all the data blocks are occupied.

So, in both the cases, the data in reserved block is written to its respective location to keep the data safe. The scattering of metadata of a file or the data of file is taken care of as the program searches for a free datablock and all the datablock numbers are saved in the 
reserved block irrespective of the sequence.

How to retrive data from the virtual disk?
Firstly, the filename is check whether its in the virtual disk or not. If found, then each datablock number in reserved block is copied to a location unless the metadata->next != -1 and  all the datablock numbers are not copied.
