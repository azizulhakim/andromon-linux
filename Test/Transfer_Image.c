#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_DEVICE      "/dev/andromon0"
#define Image_File			"test.png"
#define DEFAULT_DURATION    800

int main(int argc, char *argv[])
{
	FILE *file;
	int deviceFileDescriptor;
	char *buffer;
	unsigned int fileLen;
	char fileSizeBuff[4];

	printf("%d\n", sizeof(unsigned int));

	// open device file
    deviceFileDescriptor = open((char*)DEFAULT_DEVICE, O_RDWR);
    if (deviceFileDescriptor == -1)
    {
        perror("open");
        exit(1);
    }

	// open image file
	file = fopen((char*)Image_File, "rb");
	if (!file){
		fprintf(stderr, "Unable to open file %s\n", (char*)Image_File);
		close(deviceFileDescriptor);
		exit(1);
	}

	// get image file length
	fseek(file, 0, SEEK_END);
	fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);

	printf("%d bytes\n", fileLen);

	// allocate buffer for image file
	buffer = (char*)malloc(fileLen + 1);
	if (!buffer){
		fprintf(stderr, "Memory error!");
		fclose(file);
		close(deviceFileDescriptor);
	}

	fileSizeBuff[0] = 0xFF & fileLen;
	fileSizeBuff[1] = 0xFF & (fileLen >> 8); 
	fileSizeBuff[2] = 0xFF & (fileLen >> 16); 
	fileSizeBuff[3] = 0xFF & (fileLen >> 24);

	printf("%d\n", fileSizeBuff[0]);
	printf("%d\n", fileSizeBuff[1]);
	printf("%d\n", fileSizeBuff[2]);
	printf("%d\n", fileSizeBuff[3]);
	write(deviceFileDescriptor, fileSizeBuff, 4);


	// read image file to buffer
	fread(buffer, fileLen, 1, file);
	fclose(file);


	// write buffer to device file
	write(deviceFileDescriptor, buffer, fileLen+1);	
    close(deviceFileDescriptor);

	free(buffer);

    return 0;
}
