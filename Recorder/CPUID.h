#pragma  once

#include <string>
#include <map>

struct CacheInfo
{
	int level; // �ڼ�������
	int size; // �����С����λKB
	int way; // ͨ����
	int linesize; // ������
	CacheInfo() // ���캯��
	{
		level = 0;
		size = 0;
		way = 0;
		linesize = 0;
	}
	CacheInfo(int clevel, int csize, int cway, int clinesize) // ���캯��
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
		//������Ϣ����ͨ��eax = 2��cpuid���õ����õ��Ĳ�����cache��Ϣ������������һЩ��Ϣ����
		//����ֵ��eax(��24λ), ebx, ecx��edx���ܹ�15��BYTE����Ϣ��ÿ��BYTE��ֵ��ͬ�����������Ҳ��ͬ��
		//������Ҫ��һ����ϣ��洢���ֲ�ͬBYTE�Ķ��壬���Զ���һ��map���͵����Ա�洢��Щ���ϡ�
		//�Ұ������Ϻͻ����йص���Ϣ�洢���£�
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
	//	3. ���CPU����������Ϣ(Vender ID String)
	std::string GetVID()
	{
		//	��eax = 0��Ϊ������������Եõ�CPU����������Ϣ��
		//cpuidָ��ִ���Ժ󣬻᷵��һ��12�ַ�����������Ϣ��ǰ�ĸ��ַ���ASC�밴��λ����λ����ebx��
		//�м��ĸ�����edx������ĸ��ַ�����ecx������˵������intel��cpu���᷵��һ����GenuineIntel�����ַ�����
		//����ֵ�Ĵ洢��ʽΪ:
		//31 23 15 07 00
		//	EBX| u (75)| n (6E)| e (65)| G (47)
		//	EDX| I (49)| e (65)| n (6E)| i (69)
		//	ECX| l (6C)| e (65)| t (74)| n (6E)
		//	��˿�������ʵ������
		char cVID[13]; // �ַ����������洢��������Ϣ
		memset(cVID, 0, 13); // ��������0
		Executecpuid(0); // ִ��cpuidָ�ʹ��������� eax = 0
		memcpy(cVID, &cpuinfo[1], 4); // ����ǰ�ĸ��ַ�������
		memcpy(cVID+4, &cpuinfo[3], 4); // �����м��ĸ��ַ�������
		memcpy(cVID+8, &cpuinfo[2], 4); // ��������ĸ��ַ�������
		return std::string(cVID); // ��string����ʽ����
	}
	//4. ���CPU�̱���Ϣ(Brand String)
	std::string GetBrand()
	{
		//���ҵĵ����ϵ���Ҽ���ѡ�����ԣ������ڴ��ڵ����濴��һ��CPU����Ϣ�������CPU���̱��ַ�����
		//CPU���̱��ַ���Ҳ��ͨ��cpuid�õ��ġ������̱���ַ����ܳ�(48���ַ�)��
		//���Բ�����һ��cpuidָ��ִ��ʱȫ���õ�������intel�����ֳ���3��������
		//eax����������ֱ���0x80000002,0x80000003,0x80000004��ÿ�η��ص�16���ַ���
		//���մӵ�λ����λ��˳�����η���eax, ebx, ecx, edx����ˣ�������ѭ���ķ�ʽ��ÿ��ִ�����Ժ󱣴�����
		//Ȼ��ִ����һ��cpuid��
		const unsigned long BRANDID = 0x80000002; // ��0x80000002��ʼ����0x80000004����
		char cBrand[49]; // �����洢�̱��ַ�����48���ַ�
		memset(cBrand, 0, 49); // ��ʼ��Ϊ0
		for (unsigned long i = 0; i < 3; i++) // ����ִ��3��ָ��
		{
			Executecpuid(BRANDID + i);
			memcpy(cBrand + i*16, &cpuinfo[0], 16); // ÿ��ִ�н����󣬱����ĸ��Ĵ������asc�뵽����
		} // �������ڴ��У�m_eax, m_ebx, m_ecx, m_edx����������
		// ���Կ���ֱ�����ڴ�copy�ķ�ʽ���б���
		return std::string(cBrand); // ��string����ʽ����
	}
	// �ж��Ƿ�֧��hyper-threading
	bool CPUID::IsHyperThreading() 
	{
		//��Щ����CPU�����ԡ�CPU�����Կ���ͨ��cpuid��ã�������eax = 1������ֵ����edx��ecx��
		//ͨ����֤edx����ecx��ĳһ��bit�����Ի��CPU��һ�������Ƿ�֧�֡�����˵��
		//edx��bit 32�����Ƿ�֧��MMX��edx��bit 28�����Ƿ�֧��Hyper-Threading��
		//ecx��bit 7�����Ƿ�֧��speed sted��������ǻ��CPU���Ե����ӣ�
		Executecpuid(1); // ִ��cpuidָ�ʹ��������� eax = 1
		return cpuinfo[3] & (1<<28); // ����edx��bit 28
	}
	


	bool IsEST() // �ж��Ƿ�֧��speed step
	{
		Executecpuid(1); // ִ��cpuidָ�ʹ��������� eax = 1
		return cpuinfo[2] & (1 <<7); // ����ecx��bit 7
	}

	bool IsMMX() // �ж��Ƿ�֧��MMX
	{
		Executecpuid(1); // ִ��cpuidָ�ʹ��������� eax = 1
		return cpuinfo[3] & (1 <<23); // ����edx��bit 23
	}
	//���CPU�Ļ���(cache)
	unsigned long CPUID::GetCacheInfo(CacheInfo& L1, CacheInfo& L2, CacheInfo& L3)
	{
		//	���棬����CACHE���Ѿ���Ϊ�ж�CPU���ܵ�һ���ָ�ꡣ������Ϣ�������ڼ�������(level)�������С(size)��ͨ����(way)��������(line size)����˿���ʹ��һ���ṹ�����洢������Ϣ��
		//�ڵõ�����ֵ�Ժ�ֻ��Ҫ����ÿһ��BYTE��ֵ���ҵ���m_cache�д��ڵ�Ԫ�أ��Ϳ��Եõ�cache��Ϣ�ˡ��������£�

		unsigned short cValues[16]; // �洢���ص�16��byteֵ
		unsigned long result = 0; // ��¼���ֵĻ�������
		Executecpuid(2); // ִ��cpuid������Ϊeax = 2
		memcpy(cValues, &cpuinfo[0], 16); // ��m_eax, m_ebx, m_ecx��m_edx�洢��cValue
		for (int i = 1; i < 16; i++) // ��ʼ������ע��eax�ĵ�һ��byteû�����壬��Ҫ����
		{
			if (m_cache.find(cValues[i]) != m_cache.end()) // �ӱ��в��Ҵ���Ϣ�Ƿ������
			{
				switch (m_cache[cValues[i]].level) // �Ժ����������滺����Ϣ
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

	bool GetSerialNumber(SerialNumber& serial)//���CPU�����к�
	{
		//���к��޴����ڣ���CPU�����к���һ��96bit�Ĵ���ʾ����ʽ��������6��WORDֵ��XXXX-XXXX-XXXX-XXX-XXXX-XXXX��WORD��16��bit�������ݣ�������unsigned shortģ�⣺
		//������к���Ҫ�������裬������eax = 1�����������ص�eax�д洢���кŵĸ�����WORD����eax = 3������������ecx��edx���ӵ�λ����λ��˳��洢ǰ4��WORD��ʵ�����£�

		Executecpuid(1); // ִ��cpuid������Ϊ eax = 1
		//bool isSupport = cpuinfo[3] & (1<<18); // edx�Ƿ�Ϊ1����CPU�Ƿ�������к�
		//if (false == isSupport) // ��֧�֣�����false
		//{
		//	return false;
		//}

		//memcpy(&serial.nibble[4], &cpuinfo[0], 4); // eaxΪ���λ������WORD

		serial.nibble[0] = cpuinfo[3];
		serial.nibble[1] = cpuinfo[0];
		Executecpuid(3); // ִ��cpuid������Ϊ eax = 3
		//memcpy(&serial.nibble[0], &cpuinfo[2], 8); // ecx �� edxΪ��λ��4��WORD.n
		serial.nibble[2] = cpuinfo[3];
		serial.nibble[4] = cpuinfo[2];
		return true;
	}
private:
	void Executecpuid(unsigned long veax) // ����ʵ��cpuid
	{
		// ��ΪǶ��ʽ�Ļ����벻��ʶ�� ���Ա����
		// ���Զ����ĸ���ʱ������Ϊ����
		//�����Ϳ���ͨ��ֱ�ӵ���Executecupid()�����ķ�ʽ��ִ��cpuidָ���ˣ�����ֵ�������Ա����m_eax, m_ebx, m_ecx��m_edx�С�
		__cpuid(cpuinfo,veax);
	
	}
	int cpuinfo[4];
	//m_cache�����Ա���������£�
	std::map<int, CacheInfo> m_cache; // Cache information table
};


