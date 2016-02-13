#ifndef BITS_MACROS_
#define BITS_MACROS_
/***********************************************************
//BITS MACROS
//PASHGAN 2009
//CHIPENABLE.RU
//
//reg : ��� ����������, ��������
//bit : ������� ����
//val : 0 ��� 1
************************************************************/
#define Bit(bit)  (1<<(bit))

#define ClearBit(reg, bit)       reg &= (~(1<<(bit)))
//������: ClearBit(PORTB, 1); //�������� 1-� ��� PORTB

#define SetBit(reg, bit)          reg |= (1<<(bit))	
//������: SetBit(PORTB, 3); //���������� 3-� ��� PORTB

//��������� ����� ��� Long
#define SetBitValLong(reg, bit, val) do{if ((long) val==0) reg &= (~((long) 1<<(bit)));\
else reg |= ((long) 1<<(bit));}while(0)
//��� int � char							
#define SetBitVal(reg, bit, val) do{if ((val&1)==0) reg &= (~(1<<(bit)));\
                                  else reg |= (1<<(bit));}while(0)
//������: SetBitVal(PORTB, 3, 1); //���������� 3-� ��� PORTB
//	  SetBitVal(PORTB, 2, 0); //�������� 2-� ��� PORTB

#define BitIsClear(reg, bit)    ((reg & (1<<(bit))) == 0)
//������: if (BitIsClear(PORTB,1)) {...} //���� ��� ������

#define BitIsSet(reg, bit)       ((reg & (1<<(bit))) != 0)
//������: if(BitIsSet(PORTB,2)) {...} //���� ��� ����������

#define InvBit(reg, bit)	  reg ^= (1<<(bit))
//������: InvBit(PORTB, 1); //������������� 1-� ��� PORTB

#endif//BITS_MACROS_



