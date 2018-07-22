/*
 * bitwise.h
 *
 *  Created on: 2018年6月18日
 *      Author: hxhoua
 *      bit operation
 */

#ifndef INCLUDE_BITWISE_OPS_H_
#define INCLUDE_BITWISE_OPS_H_



#define reversebit(x,y)  x^(1<<y) //指定的某一位数取反
#define getbit(x,y)   ((x) >> (y)&1) //获取的某一位的值
#define getLastybits(x,y)    x&((1<<y)-1)//取最后N位
#define getYbits1Value(y)    (1<<y)-1//2的y次方-1
#define getLastybitsReverse(x,y)    ~x&((1<<y)-1)//取反后的最后几位
#define setbit(x,y)  x|=(1<<y) //指定的某一位数置1
#define clrbit(x,y)  x&=~(1<<y) //指定的某一位数置0

#endif /* INCLUDE_BITWISE_OPS_H_ */
