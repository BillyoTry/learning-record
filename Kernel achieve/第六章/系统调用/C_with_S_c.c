extern void asm_print(char*,int);
void c_print(char* str) {
    int len=0;
    while(str[len++]);   // 循环求出长度len,以'\0'结尾
    asm_print(str, len);
}