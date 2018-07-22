/*
 * bitwise.h
 *
 *  Created on: 2018��6��18��
 *      Author: hxhoua
 *      bit operation
 */

#ifndef INCLUDE_BITWISE_OPS_H_
#define INCLUDE_BITWISE_OPS_H_



#define reversebit(x,y)  x^(1<<y) //ָ����ĳһλ��ȡ��
#define getbit(x,y)   ((x) >> (y)&1) //��ȡ��ĳһλ��ֵ
#define getLastybits(x,y)    x&((1<<y)-1)//ȡ���Nλ
#define getYbits1Value(y)    (1<<y)-1//2��y�η�-1
#define getLastybitsReverse(x,y)    ~x&((1<<y)-1)//ȡ��������λ
#define setbit(x,y)  x|=(1<<y) //ָ����ĳһλ����1
#define clrbit(x,y)  x&=~(1<<y) //ָ����ĳһλ����0

#endif /* INCLUDE_BITWISE_OPS_H_ */
