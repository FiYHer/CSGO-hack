#pragma once
#include <iostream>
#include <Windows.h>

constexpr int ShellCodeLen = 5;			///跳转指令 + 函数地址
constexpr char Asmjmp = '\xe9';		///jmp

class InlineHook
{
private:
	unsigned char m_pOldAddress[ShellCodeLen];			///原始地址
	unsigned char m_pNewAddress[ShellCodeLen];		///新的地址

	int m_nOldAddress;			///原始地址
	int m_nNewAddress;		///新的地址

public:
	InlineHook();
	~InlineHook();

	///Note 初始化Hook操作
	///nOldAddress 我们想要Hook住的函数地址
	///nNewAddress 我们自己定义的函数地址
	///执行成功返回true，否则返回false
	bool Initialize(int nOldAddress,int nNewAddress);

	///Note 修改函数地址实现Hook操作
	///执行成功返回true，失败返回false
	bool ModifyAddress();

	///Note 还原函数地址
	///执行成功返回true，否则返回false
	bool ReduceAddress();

};

