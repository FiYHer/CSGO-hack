#include "InlineHook.h"

InlineHook::InlineHook():m_nOldAddress(0),m_nNewAddress(0)
{
	memset(m_pOldAddress, 0, ShellCodeLen);
	memset(m_pNewAddress, 0, ShellCodeLen);
	m_pNewAddress[0] = Asmjmp;		///构造指令跳去新的函数地址
}

InlineHook::~InlineHook(){}

bool InlineHook::Initialize(int nOldAddress, int nNewAddress)
{
	try
	{
		///保存新的函数地址
		int nAddressOffset = nNewAddress - (nOldAddress + ShellCodeLen);
		memcpy(&m_pNewAddress[1], &nAddressOffset, ShellCodeLen - 1);

		DWORD dwOldProtect;		///将原始函数地址的内存模式修改为可读可写可执行
		if (VirtualProtect(reinterpret_cast<void*>(nOldAddress), ShellCodeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		///保存原始地址
		memcpy(m_pOldAddress, reinterpret_cast<void*>(nOldAddress), ShellCodeLen);

		///将原始函数地址的内存模式还原
		if (VirtualProtect(reinterpret_cast<void*>(nOldAddress), ShellCodeLen, dwOldProtect, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		m_nOldAddress = nOldAddress;			///保存原始函数地址
		m_nNewAddress = nNewAddress;		///保存新的函数地址
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool InlineHook::ModifyAddress()
{
	try
	{
		DWORD dwOldProtect;		///将原始函数地址的内存模式修改为可读可写可执行
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		///将新函数地址的字节数据复制到原始函数地址，实现inline hook操作
		memcpy(reinterpret_cast<void*>(m_nOldAddress), m_pNewAddress, ShellCodeLen);

		///将原始函数地址的内存模式还原
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, dwOldProtect, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool InlineHook::ReduceAddress()
{
	try
	{
		DWORD dwOldProtect;		///将原始函数地址的内存模式修改为可读可写可执行
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		///将原始函数的字节数据还原，实现去inline hook
		memcpy(reinterpret_cast<void*>(m_nOldAddress), m_pOldAddress, ShellCodeLen);

		///将原始函数地址的内存模式还原
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, dwOldProtect, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}
