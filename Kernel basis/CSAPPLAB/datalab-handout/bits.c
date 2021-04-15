/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
    //x=0011   y=1100     x^y=1111
    //~x=1100  ~y=0011    (~x)&(~y)=0000
    //~((~x)&(~y))=1111
  //return ~((~x)&(~y));错误
    return ~(~x&~y)&~(x&y);
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
//1000 0000 0000 0000 0000 0000 0000 0000 为32位补码最小值
//将1移动到最左边
  return 1<<31;

}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
//0111 1111 1111 1111 1111 1111 1111 1111 为32位补码最大值
//当x为最大值0x7fffffff时,x+1=2147483648,x^(x+1)=0xffffffff,~(x^(x+1))=0,!(~(x^(x+1)))=1
//!!(x+1)是为了排除0xffffffff
    return !(~(x^(x+1)))&!!(x+1);
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
//0xFFFFFFFD=1111 1111 1111 1111 1111 1111 1111 1101
//0xAAAAAAAA=1010 1010 1010 1010 1010 1010 1010 1010
//0x55555555=0101 0101 0101 0101 0101 0101 0101 0101
//0x55=85=0101 0101
//85<<8=0101 0101 0000 0000
//85<<16=0101 0101 0000 0000 0000 0000
//85<<24=0101 0101 0000 0000 0000 0000 0000 0000
//0x55555555=85+(85<<8)+(85<<16)+(85<<24)
//x|0x55555555 利用|运算将偶数位置1，这样就可以忽略考虑偶数位，如果x的奇数位也为1的话
//那么最后的结果为1111 1111 1111 1111 1111 1111 1111 1111
    return !(~(x|(85+(85<<8)+(85<<16)+(85<<24))));
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return (~x+1);
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
//数字的ascii码范围在0x30~0x39之间
//    x-48>=0 & x-58<0
//    (x+~58+1)>>31 if x>=58，return 0，or return -1
//    !!((x+~58+1)>>31)这里的!!不管有没有似乎从结果上来看都是一样的，他把所有非零的数转换成1，显得更严谨
    return !((x+~48+1)>>31)&!!((x+~58+1)>>31);
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
//这里只有两种情况第一种是 y 0；第二种是0 z，这里可以利用|，y|0=y，0|z=z，和0xffffffff&int=int及（-x）|x=-1(x为奇数)
//首先想到的就是  （①&y）|（②&z）
//当x=0时,~x=0xffffffff,~x+1=0(溢出),(~x+1)|x=x=0,0&y=0;~((~x+1)|x)=0xffffffff,~((~x+1)|x)&z=z
//当x！=0时,~x+1=-x,(~x+1)|x=-1=0xffffffff(32位),(((~x+1)|x)&y)=y;~((~x+1)|x)=0,0&z=0
    //return ((((~x+1)|x)&y)|(~((~x+1)|x)&z));错误
    x = !!x;
    x = ~x+1;
    return (x&y)|(~x&z);//将x的布尔值转化为0或1来解决
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
//((x+~58+1)>>31);if x>=58，return 0，or return -1;
//!((y+~x+1)>>31);if y>=x，return 0，or return -1;
  //return !((y+~x+1)>>31);
    int negX=~x+1;//-x
    int addX=negX+y;//y-x
    int checkSign = addX>>31&1; //y-x的符号
    int leftBit = 1<<31;//最大位为1的32位有符号数
    int xLeft = x&leftBit;//x的符号
    int yLeft = y&leftBit;//y的符号
    int bitXor = xLeft ^ yLeft;//x和y符号相同标志位，相同为0不同为1
    bitXor = (bitXor>>31)&1;//符号相同标志位格式化为0或1
    return ((!bitXor)&(!checkSign))|(bitXor&(xLeft>>31));
    //两数(正负)符号相同且y-x>=0;
    //两数(正负)符号不同且x的符号位为1(x为负);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
//反向绝对值的方案
//当x=0,~x=0xffffffff,~x+1=0,x|(~x+1)=0,(x|(~x+1))>>31=0,((x|(~x+1))>>31)+1=1
//当x=int,~x+1=-int,x|(~x+1)=int|-int=-int,(x|(~x+1))>>31=-int>>31=-1,((x|(~x+1))>>31)+1=0
//这里的关键就是int|-int(int为偶数)=-int(这里的结果一定是负，保证后面算数右移31后所有位为1,即-1)
  return ((x|(~x+1))>>31)+1;;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
    int b16,b8,b4,b2,b1,b0;
    int sign=x>>31;//if x>=0 return 0,or return -1
    x = (sign&~x)|(~sign&x);//如果x为正则不变，否则按位取反,方便找负数的最高0位
    b16 = !!(x>>16)<<4;//高十六位是否有1，如果有1则!!(>>16)为1，否则为0
    x = x>>b16;//如果有（至少需要16位），则将原数右移16位
    b8 = !!(x>>8)<<3;//剩余位高8位是否有1
    x = x>>b8;//如果有（至少需要16+8=24位），则右移8位
    b4 = !!(x>>4)<<2;//同理
    x = x>>b4;
    b2 = !!(x>>2)<<1;
    x = x>>b2;
    b1 = !!(x>>1);
    x = x>>b1;
    b0 = x;
    return b16+b8+b4+b2+b1+b0+1;//+1表示加上符号位
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
    //unsigned S = uf&0x80000000;// 1000 0000 0000 0000 0000 0000 0000 0000
    //unsigned E = uf&0x7f800000;// 0111 1111 1000 0000 0000 0000 0000 0000
    //unsigned Sign = uf&0x007fffff;// 0000 0000 0111 1111 1111 1111 1111 1111
    int exp_mask = 0x7f800000;//0[11111111]0..0 作为指数部分的掩码
    int anti_exp_maks = 0x807fffff;//1[00000000]1..1 作为除指数之外部分的掩码
    int E = ((uf & exp_mask) >> 23);//指数,>>23将指数部分作为一个单独是数来运算
    int Sign = uf & 0x80000000;//符号位
    if(E == 0) return (uf << 1) | Sign;//非规格化数：直接左移一位(乘2)，并将符号位重置
    if(E == 255) return uf;//无穷大或 NaN ：直接返回
    //规格化数：指数部分加 1 ，再做溢出判定
    E++;
    if(E == 255) return exp_mask | Sign;//无穷
    return (E << 23) | (uf & anti_exp_maks);//将各部分合并
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
    int S=uf>>31;
    int E=((uf&0x7f800000)>>23)-127;
    int frac=(uf&0x007fffff)|0x00800000;
    if(!(uf&0x7fffffff)) return 0;

    if(E>31) return 0x80000000;
    if(E<0) return 0;

    if(E>23) frac<<=(E-23);
    else frac>>=(23-E);

    if(!((frac>>31)^S)) return frac;
    else if(frac>>31) return 0x80000000;
    else return ~frac+1;
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
    int E=x+127;
    if(E<=0) return 0; //too small
    if(E>=255) return 0xFF<<23; //too large
    return E<<23;
}

