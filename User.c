#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

// to append two string
char * append (char * string1,char * string2)
{
    char *dest =(char *)malloc(strlen(string1) + strlen(string2) + 2);
    char *src [50];
    strcpy(dest,string1);// make copy from string1
    strcpy(src,string2);// make copy from string2
    strcat(dest, src);// add string1 to string2
    return dest;// to return dest declared above
}
void main (int argc, char** argv[])
{
    char * path = "/sys/kernel/kobj_module";
    char * led =append("/",argv[2]);
    char *fullPath = append(path,led);
    if(argc==3)  // ./leds get caps
    {
        fullPath=append("cat ",fullPath);
        // make system call to get state
        system(fullPath);
    }
    else if(argc==4)   // ./leds set caps on
    {
        if(strcmp(argv[3],"on")==0)
        {
            fullPath=append("echo 1 >> ",fullPath);
        }
        else if (strcmp(argv[3],"off")==0)
        {
            fullPath=append("echo 0 >> ",fullPath);
        }
        // make system call to set state
        system(fullPath);
    }
}
