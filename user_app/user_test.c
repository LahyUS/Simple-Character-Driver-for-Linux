#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

typedef struct 
{
    unsigned char read_count_l; 
    unsigned char read_count_h;
    unsigned char write_count_l;
    unsigned char write_count_h;
    unsigned char device_status;
} status_t;

#define BUFFER_SIZE 1024
#define DEVICE_NODE "/dev/char_device_file"

/* Define ioctl commands code */
#define MAGICAL_NUMBER 243
#define CLEAR_DATA_CHARDEV _IO(MAGICAL_NUMBER, 0)
#define GET_STATUS_CHARDEV _IOR(MAGICAL_NUMBER, 1, status_t *)
#define CTRL_READ_CHARDEV  _IOW(MAGICAL_NUMBER, 2, unsigned char *)
#define CTRL_WRITE_CHARDEV _IOW(MAGICAL_NUMBER, 3, unsigned char *)

/* Function: Entry point OPEN of char driver */
int open_chardev() 
{
    // System call for opening device file
    int fd = open(DEVICE_NODE, O_RDWR);

    // Check result
    if(fd < 0) 
    {
        printf("Can not open the device file\n");
        exit(1);
    }

    return fd;
}

/* Function: Entry point RELEASE of char driver */
void close_chardev(int fd) 
{
    // System call for closing device file
    close(fd);
}

/* Function: Entry point READ of char driver*/
void read_data_chardev()
{
    int ret = 0;
    char user_buf[BUFFER_SIZE];

    int fd = open_chardev(); // System call for opening device file
    ret = read(fd, user_buf, BUFFER_SIZE); // System call for reading data from device file
    close_chardev(fd); // System call for closing device file


    // Check result
    if(ret < 0)
        printf("Could not read a message from %s\n", DEVICE_NODE);
    else 
        printf("Read a message from %s: %s\n", DEVICE_NODE, user_buf);
}

/* Function: Entry point WRITE of char driver*/
void write_data_chardev()
{
    int ret = 0;
    char user_buf[BUFFER_SIZE];
    printf("Enter your messsage: ");
    scanf(" %[^\n]s", user_buf);

    int fd = open_chardev(); // System call for opening device file
    ret = write(fd, user_buf, strlen(user_buf) + 1); // System call for writing the message into device file
    close_chardev(fd); // System call for closing device file

    // Check result
    if(ret < 0)
        printf("Could not write the message to %s\n", DEVICE_NODE);
    else 
        printf("Wrote the message to %s\n", DEVICE_NODE);
}

/* Function: Entry point CLEAR of char driver*/
void clear_data_chardev()
{
    int fd = open_chardev(); // open device file
    int ret = ioctl(fd, CLEAR_DATA_CHARDEV); // clear data registers
    close_chardev(fd); // close device file
    printf("%s data registers in char device\n", (ret < 0)?"Could not clear":"Clear");
}

/* Function: Entry point CONTROL_READ of char driver*/
void control_read_chardev()
{   
    unsigned char isReadable = 0;
    status_t status;
    char c = 'n';
    printf("Do you want to enable reading from data registers (y/n)? ");
    scanf(" %c", &c);
    
    if(c == 'y')
        isReadable = 1;
    else if(c == 'n')
        isReadable = 0;
    else 
        return;

    int fd = open_chardev(); // open device file
    ioctl(fd, CTRL_READ_CHARDEV, (unsigned char*)&isReadable); // set permission
    ioctl(fd, GET_STATUS_CHARDEV, (status_t*)&status); // get status from  status registers
    close_chardev(fd); // close device file
    
    if(status.device_status & 0x01)
        printf("Enable to read from data registers successful\n");
    else 
        printf("Disable to read from data registers successful\n");
}

/* Function: Entry point CONTROL_WRITE of char driver*/
void control_write_chardev()
{   
    unsigned char isWriteable = 0;
    status_t status;
    char c = 'n';
    printf("Do you want to enable writing from data registers (y/n)? ");
    scanf(" %c", &c);
    
    if(c == 'y')
        isWriteable = 1;
    else if(c == 'n')
        isWriteable = 0;
    else 
        return;

    int fd = open_chardev(); // open device file
    ioctl(fd, CTRL_WRITE_CHARDEV, (unsigned char*)&isWriteable); // set permission
    ioctl(fd, GET_STATUS_CHARDEV, (status_t*)&status); // get status from status registers
    close_chardev(fd); // close device file
    
    if(status.device_status & 0x01)
        printf("Enable to write to data registers successful\n");
    else 
        printf("Disable to write to data registers successful\n");
}   

/* Function: Entry point GET_STATUS of char driver*/
void get_status_chardev()
{
    status_t status;
    unsigned int read_cnt, write_cnt;

    int fd = open_chardev(); // open device file
    ioctl(fd, GET_STATUS_CHARDEV, (status_t*)&status); // get status from status registers
    close_chardev(fd); // close device file
    
    switch(status.device_status)
    {
        case 0:
            printf("Current status:\n\tWriting: Disable - Reading: Disable\n");
            break;
        case 1:
            printf("Current status:\n\tWriting: Disable - Reading: Enable\n");
            break;
        case 2:
            printf("Current status:\n\tWriting: Enable - Reading: Disable\n");
            break;
        case 3:
            printf("Current status:\n\tWriting: Enable - Reading: Enable\n");
            break;
        default: 
            break;
    }

    read_cnt = status.read_count_h << 8 | status.read_count_l; //  calculate reading time
    write_cnt = status.write_count_h << 8 | status.write_count_l; // calculate writing time
    printf("Statistic: number of reading(%u), number of writing (%u)\n", read_cnt, write_cnt);
}

int main() 
{
    int ret = 0;
    char option = 'q';
    int fd = -1;

    /* DRIVER MENU */
    printf("Select below options:\n");
    printf("\to (to open a device node)\n");
    printf("\tc (to close the device node)\n");
    printf("\tr (to read data from the device node)\n");
    printf("\tw (to write data to the device node)\n");
    printf("\tC (to clear data registers)\n");
    printf("\tR (to enable/disable to read from data registers)\n");
    printf("\tW (to enable/disable to write to data registers)\n");
    printf("\ts (to get status of device)\n");
    printf("\tq (to quit the application)\n");
    while (1) 
    {
        printf("Enter your option: ");
        scanf(" %c", &option);

        switch (option) 
        {
            case 'o':
                if (fd < 0)
                    fd = open_chardev();
                else
                    printf("%s has already opened\n", DEVICE_NODE);
                break;
            case 'c':
                if (fd > -1)
                    close_chardev(fd);
                else
                    printf("%s has not opened yet! Can not close\n", DEVICE_NODE);
                fd = -1;
                break;
            case 'r':
                read_data_chardev();
                break; 
            case 'w':
                write_data_chardev();
                break;
            case 'C':
                clear_data_chardev();   
                break;
            case 'R':
                control_read_chardev();  
                break;
            case 'W':
                control_write_chardev(); 
                break;
            case 's':
                get_status_chardev();    
                break; 
            case 'q':
                if (fd > -1)
                    close_chardev(fd);
                printf("Quit the application. Good bye!\n");
                return 0;
            default:
                printf("invalid option %c\n", option);
                break;
        }
    };
}
