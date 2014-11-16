#pragma  once

#include <string>
#include <map>

struct CacheInfo
{
	int level; // 第几级缓存
	int size; // 缓存大小，单位KB
	int way; // 通道数
	int linesize; // 吞吐量
	CacheInfo() // 构造函数
	{
		level = 0;
		size = 0;
		way = 0;
		linesize = 0;
	}
	CacheInfo(int clevel, int csize, int cway, int clinesize) // 构造函数
	{
		level = clevel;
		size = csize;
		way = cway;
		linesize = clinesize;
	}
};

struct SerialNumber{
	unsigned long nibble[6];
};
class CPUID
{

public:
	CPUID(){
		//缓存信息可以通过eax = 2的cpuid来得到（得到的不光有cache信息，还有其他的一些信息），
		//返回值在eax(高24位), ebx, ecx和edx，总共15个BYTE的信息，每个BYTE的值不同，代表的意义也不同，
		//所以需要用一个哈希表存储各种不同BYTE的定义，可以定义一个map类型的类成员存储这些资料。
		//我把资料上和缓存有关的信息存储如下：
		m_cache[0x06] = CacheInfo(1, 8, 4, 32);
		m_cache[0x08] = CacheInfo(1, 16, 4, 32);
		m_cache[0x0a] = CacheInfo(1, 8, 2, 32);
		m_cache[0x0c] = CacheInfo(1, 16, 4, 32);
		m_cache[0x2c] = CacheInfo(1, 32, 8, 64);
		m_cache[0x30] = CacheInfo(1, 32, 8, 64);
		m_cache[0x60] = CacheInfo(1, 16, 8, 64);
		m_cache[0x66] = CacheInfo(1, 8, 4, 64);
		m_cache[0x67] = CacheInfo(1, 16, 4, 64);
		m_cache[0x68] = CacheInfo(1, 32, 4, 64);
		m_cache[0x39] = CacheInfo(2, 128, 4, 64);
		m_cache[0x3b] = CacheInfo(2, 128, 2, 64);
		m_cache[0x3c] = CacheInfo(2, 256, 4, 64);
		m_cache[0x41] = CacheInfo(2, 128, 4, 32);
		m_cache[0x42] = CacheInfo(2, 256, 4, 32);
		m_cache[0x43] = CacheInfo(2, 512, 4, 32);
		m_cache[0x44] = CacheInfo(2, 1024, 4, 32);
		m_cache[0x45] = CacheInfo(2, 2048, 4, 32);
		m_cache[0x79] = CacheInfo(2, 128, 8, 64);
		m_cache[0x7a] = CacheInfo(2, 256, 8, 64);
		m_cache[0x7b] = CacheInfo(2, 512, 8, 64);
		m_cache[0x7c] = CacheInfo(2, 1024, 8, 64);
		m_cache[0x82] = CacheInfo(2, 256, 8, 32);
		m_cache[0x83] = CacheInfo(2, 512, 8, 32);
		m_cache[0x84] = CacheInfo(2, 1024, 8, 32);
		m_cache[0x85] = CacheInfo(2, 2048, 8, 32);
		m_cache[0x86] = CacheInfo(2, 512, 4, 64);
		m_cache[0x87] = CacheInfo(2, 1024, 8, 64);
		m_cache[0x22] = CacheInfo(3, 512, 4, 64);
		m_cache[0x23] = CacheInfo(3, 1024, 8, 64);
		m_cache[0x25] = CacheInfo(3, 2048, 8, 64);
		m_cache[0x29] = CacheInfo(3, 4096, 8, 64);
	}
	//	3. 获得CPU的制造商信息(Vender ID String)
	std::string GetVID()
	{
		//	把eax = 0作为输入参数，可以得到CPU的制造商信息。
		//cpuid指令执行以后，会返回一个12字符的制造商信息，前四个字符的ASC码按低位到高位放在ebx，
		//中间四个放在edx，最后四个字符放在ecx。比如说，对于intel的cpu，会返回一个“GenuineIntel”的字符串，
		//返回值的存储格式为:
		//31 23 15 07 00
		//	EBX| u (75)| n (6E)| e (65)| G (47)
		//	EDX| I (49)| e (65)| n (6E)| i (69)
		//	ECX| l (6C)| e (65)| t (74)| n (6E)
		//	因此可以这样实现他：
		char cVID[13]; // 字符串，用来存储制造商信息
		memset(cVID, 0, 13); // 把数组清0
		Executecpuid(0); // 执行cpuid指令，使用输入参数 eax = 0
		memcpy(cVID, &cpuinfo[1], 4); // 复制前四个字符到数组
		memcpy(cVID+4, &cpuinfo[3], 4); // 复制中间四个字符到数组
		memcpy(cVID+8, &cpuinfo[2], 4); // 复制最后四个字符到数组
		return std::string(cVID); // 以string的形式返回
	}
	//4. 获得CPU商标信息(Brand String)
	std::string GetBrand()
	{
		//在我的电脑上点击右键，选择属性，可以在窗口的下面看到一条CPU的信息，这就是CPU的商标字符串。
		//CPU的商标字符串也是通过cpuid得到的。由于商标的字符串很长(48个字符)，
		//所以不能在一次cpuid指令执行时全部得到，所以intel把它分成了3个操作，
		//eax的输入参数分别是0x80000002,0x80000003,0x80000004，每次返回的16个字符，
		//按照从低位到高位的顺序依次放在eax, ebx, ecx, edx。因此，可以用循环的方式，每次执行完以后保存结果，
		//然后执行下一次cpuid。
		const unsigned long BRANDID = 0x80000002; // 从0x80000002开始，到0x80000004结束
		char cBrand[49]; // 用来存储商标字符串，48个字符
		memset(cBrand, 0, 49); // 初始化为0
		for (unsigned long i = 0; i < 3; i++) // 依次执行3个指令
		{
			Executecpuid(BRANDID + i);
			memcpy(cBrand + i*16, &cpuinfo[0], 16); // 每次执行结束后，保存四个寄存器里的asc码到数组
		} // 由于在内存中，m_eax, m_ebx, m_ecx, m_edx是连续排列
		// 所以可以直接以内存copy的方式进行保存
		return std::string(cBrand); // 以string的形式返回
	}
	// 判断是否支持hyper-threading
	bool CPUID::IsHyperThreading() 
	{
		//这些都是CPU的特性。CPU的特性可以通过cpuid获得，参数是eax = 1，返回值放在edx和ecx，
		//通过验证edx或者ecx的某一个bit，可以获得CPU的一个特性是否被支持。比如说，
		//edx的bit 32代表是否支持MMX，edx的bit 28代表是否支持Hyper-Threading，
		//ecx的bit 7代表是否支持speed sted。下面就是获得CPU特性的例子：
		Executecpuid(1); // 执行cpuid指令，使用输入参数 eax = 1
		return cpuinfo[3] & (1<<28); // 返回edx的bit 28
	}
	


	bool IsEST() // 判断是否支持speed step
	{
		Executecpuid(1); // 执行cpuid指令，使用输入参数 eax = 1
		return cpuinfo[2] & (1 <<7); // 返回ecx的bit 7
	}

	bool IsMMX() // 判断是否支持MMX
	{
		Executecpuid(1); // 执行cpuid指令，使用输入参数 eax = 1
		return cpuinfo[3] & (1 <<23); // 返回edx的bit 23
	}
	//获得CPU的缓存(cache)
	unsigned long CPUID::GetCacheInfo(CacheInfo& L1, CacheInfo& L2, CacheInfo& L3)
	{
		//	缓存，就是CACHE，已经成为判断CPU性能的一项大指标。缓存信息包括：第几级缓存(level)，缓存大小(size)，通道数(way)，吞吐量(line size)。因此可以使用一个结构体来存储缓存信息。
		//在得到返回值以后，只需要遍历每一个BYTE的值，找到在m_cache中存在的元素，就可以得到cache信息了。代码如下：

		unsigned short cValues[16]; // 存储返回的16个byte值
		unsigned long result = 0; // 记录发现的缓存数量
		Executecpuid(2); // 执行cpuid，参数为eax = 2
		memcpy(cValues, &cpuinfo[0], 16); // 把m_eax, m_ebx, m_ecx和m_edx存储到cValue
		for (int i = 1; i < 16; i++) // 开始遍历，注意eax的第一个byte没有意义，需要跳过
		{
			if (m_cache.find(cValues[i]) != m_cache.end()) // 从表中查找此信息是否代表缓存
			{
				switch (m_cache[cValues[i]].level) // 对号入座，保存缓存信息
				{
				case 1: // L1 cache
					L1 = m_cache[cValues[i]];
					break;
				case 2: // L2 cache
					L2 = m_cache[cValues[i]];
					break;
				case 3: // L3 cache
					L3 = m_cache[cValues[i]];
					break;
				default:
					break;
				}
				result++;
			}
		}
		return result;
	}

	bool GetSerialNumber(SerialNumber& serial)//获得CPU的序列号
	{
		//序列号无处不在！！CPU的序列号用一个96bit的串表示，格式是连续的6个WORD值：XXXX-XXXX-XXXX-XXX-XXXX-XXXX。WORD是16个bit长的数据，可以用unsigned short模拟：
		//获得序列号需要两个步骤，首先用eax = 1做参数，返回的eax中存储序列号的高两个WORD。用eax = 3做参数，返回ecx和edx按从低位到高位的顺序存储前4个WORD。实现如下：

		Executecpuid(1); // 执行cpuid，参数为 eax = 1
		//bool isSupport = cpuinfo[3] & (1<<18); // edx是否为1代表CPU是否存在序列号
		//if (false == isSupport) // 不支持，返回false
		//{
		//	return false;
		//}

		//memcpy(&serial.nibble[4], &cpuinfo[0], 4); // eax为最高位的两个WORD

		serial.nibble[0] = cpuinfo[3];
		serial.nibble[1] = cpuinfo[0];
		Executecpuid(3); // 执行cpuid，参数为 eax = 3
		//memcpy(&serial.nibble[0], &cpuinfo[2], 8); // ecx 和 edx为低位的4个WORD.n
		serial.nibble[2] = cpuinfo[3];
		serial.nibble[4] = cpuinfo[2];
		return true;
	}
private:
	void Executecpuid(unsigned long veax) // 用来实现cpuid
	{
		// 因为嵌入式的汇编代码不能识别 类成员变量
		// 所以定义四个临时变量作为过渡
		//这样就可以通过直接调用Executecupid()函数的方式来执行cpuid指令了，返回值存在类成员变量m_eax, m_ebx, m_ecx和m_edx中。
		__cpuid(cpuinfo,veax);
	
	}
	int cpuinfo[4];
	//m_cache是类成员，定义如下：
	std::map<int, CacheInfo> m_cache; // Cache information table
};


