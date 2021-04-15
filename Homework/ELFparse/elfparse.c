#include <iostream>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

//typedef struct
//{
//    unsigned char e_ident[EI_NIDENT];     /* Magic number and other info */
//    Elf32_Half    e_type;                 /* Object file type */
//    Elf32_Half    e_machine;              /* Architecture */
//    Elf32_Word    e_version;              /* Object file version */
//    Elf32_Addr    e_entry;                /* Entry point virtual address */ //程序入口的虚拟地址
//    Elf32_Off     e_phoff;                /* Program header table file offset */   从文件头到文件头表的偏移字节数
//    Elf32_Off     e_shoff;                /* Section header table file offset */  从文件头到节区头部表格的偏移字节数
//    Elf32_Word    e_flags;                /* Processor-specific flags */
//    Elf32_Half    e_ehsize;               /* ELF header size in bytes */
//    Elf32_Half    e_phentsize;            /* Program header table entry size */ 程序头表的表项大小
//    Elf32_Half    e_phnum;                /* Program header table entry count */ 程序头表的表项数目
//    Elf32_Half    e_shentsize;            /* Section header table entry size */ 节区表的表项大小
//    Elf32_Half    e_shnum;                /* Section header table entry count */ 节区表的表项数目
//    Elf32_Half    e_shstrndx;             /* Section header string table index */
//} Elf32_Ehdr;

//typedef struct
//{
//  Elf32_Word	sh_name;		/* Section name (string tbl index) */    是节区头部字符串表节区的索引.
//  Elf32_Word	sh_type;		/* Section type */
//  Elf32_Word	sh_flags;		/* Section flags */
//  Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
//  Elf32_Off	sh_offset;		/* Section file offset */   节区的第一个字节与文件头之间的偏移
//  Elf32_Word	sh_size;		/* Section size in bytes */
//  Elf32_Word	sh_link;		/* Link to another section */
//  Elf32_Word	sh_info;		/* Additional section information */
//  Elf32_Word	sh_addralign;		/* Section alignment */
//  Elf32_Word	sh_entsize;		/* Entry size if section holds table */
//} Elf32_Shdr;

//typedef struct
//{
//  Elf32_Word	st_name;		/* Symbol name (string tbl index) */
//  Elf32_Addr	st_value;		/* Symbol value */
//  Elf32_Word	st_size;		/* Symbol size */
//  unsigned char	st_info;		/* Symbol type and binding */
//  unsigned char	st_other;		/* Symbol visibility */
//  Elf32_Section	st_shndx;		/* Section index */
//} Elf32_Sym;

int main(int argc,char * argv[]){
    int fd = open(argv[1],O_RDONLY,0);
    //设置文件偏移为文件长度(指向结尾)
    long int end = lseek(fd,0,SEEK_END);
    //设置文件偏移为开头(防止read出错)
    long int begin = lseek(fd,0,SEEK_SET);
    char* buf = (char *)malloc(end);
    read(fd,buf,end);

    //ELF头节表
    ELF64_Ehdr* header = (Elf64_Ehdr*)buf;
    //节名字符串表在节表中的下标
    Elf64_Half eshstrndx = header->e_shstrndx;  //typedef uint16_t Elf64_Half;
    //节头表
    Elf64_Shdr* section_header = (Elf64_Shdr*)(buf + header->e_shoff);
    //节名字符串表
    Elf64_Shdr* shstr = (Elf64_Shdr*)(section_headr + eshstrndx);
    //存储节名字符串的节区内容
    char* shstrbuff = (char *)(buf + shstr->sh_offset);
    //存储函数的个数
    int num = 0;

    cout << endl;
    //循环的限制条件为:e_shnum节区个数
    for(int i = 0;i<header->e_shnum;++i){
        //sh_name是一个数字相当于索引，是节名字符串表中的索引，这里检验符号表
        if(!strcmp(section_header[i].sh_name + shstrbuff,".symtab")){
            //符号表的具体位置
            Elf64_Sym* symbol_table = (Elf64_Sym*)(buf+section_header[i].sh_offset);
            //根据节表大小和节表项大小计算出一共包含多少项
            int ncount = section.header[i].sh_size / section_header[i].sh_entsize;
            //Linux 中的 ELF 文件中该项指向符号表中符号所对应的字符串节区在 Section Header Table 中的偏移
            //也就是取该字符串节区的内容
            char * str_buf = (char*)((section_header + section_header[i].sh_link)->sh_offset + buf);

            for(int i=0; i<ncount ; ++i){
                //检验符号表的类型，这里 & 0xf，是根据elf.h文件得到的
                if(((symbol_table[i].st_info) & 0xf)==STT_FUNC){
                    //st_name相当于字符串节区的索引
                    //这里就是取相应的函数名
                    cout << "function" << num+1 << ":\t" << symbol_table[i].st_name + str_buf <<endl;
                    num++;
                }
            }
            cout << "\n" << "There are" << num << "function in total" <<endl;
        }
    }
}