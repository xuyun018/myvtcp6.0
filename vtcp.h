#ifndef VTCP_H
#define VTCP_H

#include <WinSock2.h>
#include <MSWSock.h>
#include <ws2ipdef.h>

#include <Windows.h>
#include <stdint.h>

//---------------------------------------------------------------------------
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
//---------------------------------------------------------------------------

#define VTCP_TIMER						(15)							//基本时钟
#define VTCP_TIMER_TIMES				(1000 / VTCP_TIMER)				//基本时钟每秒次数
#define VTCP_ASYNC_CACHE_COUNT			(32)							//异步数量
#define VTCP_ASYNC_CACHE_COUNT_MASK		(VTCP_ASYNC_CACHE_COUNT - 1)		//异步数量掩码
#define VTCP_PACKET_CACHE_COUNT			(128)							//包缓存
#define VTCP_PACKET_CACHE_COUNT_MASK	(VTCP_PACKET_CACHE_COUNT - 1)	//包缓存掩码
#define VTCP_PACKET_GROUP_COUNT			(3)

#define VTCP_CALC_SN_INTERVAL(s1,s2)	(int(DWORD(s1)-DWORD(s2)))		//计算序列号间隔
#define VTCP_CALC_RTO(r)				(3 * (r) + VTCP_TIMER)			//计算包超时时间
#define VTCP_CALC_S2W(s,r)				(s * (r + 1) / 65536)			//计算速度到窗口
#define VTCP_CALC_S2B(s)				(s * (1000 ) / 65536)			//计算速度到带宽
#define VTCP_CALC_B2S(b)				(b * (65535) / 1000)			//计算带宽到速度

//包命令
enum VTCP_PKTCMD
{
	VTCP_PKTCMD_CONNECT = (0x0010),//连接
	VTCP_PKTCMD_CONNECT_ACK = (0x0011),//连接接收
	VTCP_PKTCMD_CONNECT_ACK_DELAY = (0x0012),//连接推迟(队列中)
	VTCP_PKTCMD_CONNECT_ACK_REFUSE = (0x0013),//连接拒绝(队列已满)
	VTCP_PKTCMD_DATA = (0x0020),//数据流
	VTCP_PKTCMD_DATA_ACK = (0x0021),//数据流应答
	VTCP_PKTCMD_RESET = (0x0030),//关闭通道
	VTCP_PKTCMD_RESET_ACK = (0x0031),//关闭通道应答
	VTCP_PKTCMD_SYNC = (0x0040),//同步控制包
	VTCP_PKTCMD_SYNC_ACK = (0x0041),//同步控制包

};


enum EVTcpPktMsk
{
	VTCP_PKTMSK_SEND = (0x0100),		//包已经发出
	VTCP_PKTMSK_SEND_REPEAT = (0x0200),		//包重复发出

};

//状态代码
enum vtcp_states
{
	VTCP_STATE_NULL = 0x00,//初始化
	VTCP_STATE_CREATED = 0x01,//创建
	VTCP_STATE_BINDED = 0x02,//绑定
	VTCP_STATE_LISTENED = 0x03,//监听
	VTCP_STATE_CONNECTING = 0x04,//连接进行
	VTCP_STATE_CONNECTED = 0x05,//连接完成
	VTCP_STATE_CONNRESET = 0x06,//连接被动断开(复位)
	VTCP_STATE_CLOSED = 0x07,//关闭

};

// SVTcpAsyncRecv
struct vtcp_buffer
{
	unsigned char *buffer;
	unsigned int length;
	unsigned int offset;
};

#pragma pack(push)
#pragma pack(1)

//4
struct vtcp_pkthdr
{
	//命令掩码
	uint16_t cmd;
	//SOCKET ID = socket 句柄
	uint16_t index;
};

#define VTCP_PACKET_DATA_SIZE (1024 - sizeof(struct vtcp_pkthdr) - 2 - 4 - 4)

//1024
struct vtcp_pktdata
{
	uint16_t ack_frequence;				//回包频率

	uint32_t tickcount;						//时间
	uint32_t sn;						//当前序号

	uint8_t data[VTCP_PACKET_DATA_SIZE];	//数据包数据
};

//14+1+32
struct vtcp_pktack
{
	uint32_t tickcount;						//时间
	uint32_t sn;						//当前序号

	//tcp_sn_recv_min
	uint16_t minimum;
	//tcp_sn_recv_max
	uint16_t maximum;
	//tcp_sn_recv_cur
	uint16_t current;

	//映像图字节大小
	uint8_t bitssize;
	//映像图
	uint8_t bits[VTCP_PACKET_CACHE_COUNT / 8];
};

//10+1+32
struct vtcp_pktsyncack
{
	uint32_t sn;

	//tcp_sn_recv_min
	uint16_t minimum;
	//tcp_sn_recv_max
	uint16_t maximum;
	//tcp_sn_recv_cur
	uint16_t current;
};

struct vtcp_pkt
{
	// 数据包头, 4 字节
	struct vtcp_pkthdr hdr;

	union
	{
		struct vtcp_pktdata data;
		struct vtcp_pktack ack;
		struct vtcp_pktsyncack synack;
	};
};

//包扩展(内存)
struct vtcp_pkt_ext
{
	//int64	speed;						//当前速度

	int cb;								//包大小（0：空包）
	int cbdata;							//包数据大小(数据包有效）
	int cboffset;						//包数据偏移(数据包有效,接收数据时使用)

	struct vtcp_pkt pkt;
};

#pragma pack(pop)

#define VTCP_SEND											1
#define VTCP_LOAD_SEND										2
#define VTCP_SENT											3
#define VTCP_RECV											4
#define VTCP_CONNECT										5
#define VTCP_ACCEPT											6
#define VTCP_LISTEN											7
#define VTCP_ADDRESSES_COMPARE								8
#define VTCP_ADDRESS_READ									9
#define VTCP_CANCEL											10
#define VTCP_TIMEOUT										11
// 申请空间
#define VTCP_REQUEST										12
// 释放空间
#define VTCP_RECYCLE										13
#define VTCP_LOCK											14
// 以下是分支
#define VTCP_LOCK_SESSION									0
#define VTCP_LOCK_DOOR										1

typedef int (WINAPI *t_vtcp_procedure)(void *parameter, const void *psession, unsigned int fd, unsigned char number, const unsigned char *address, unsigned int addresssize, void **packet, unsigned char *buffer, unsigned int bufferlength);

struct vtcp_queue
{
	unsigned char queue[sizeof(struct vtcp_buffer) * VTCP_ASYNC_CACHE_COUNT];

	unsigned int index;
	unsigned int count;
};

struct vtcp_door
{
	//

	unsigned char address[20];
};

struct vtcp
{
	// 最多128 个监听, 实际上使用通常已经太多了
	struct vtcp_door doors[128];
	unsigned int door_count;

	void **sessions;
	unsigned int count;
	unsigned int maximum;

	void *parameter;

	t_vtcp_procedure p_procedure;

	unsigned int fd;
};

void vtcp_initialize(struct vtcp *pvtcp, void *parameter, t_vtcp_procedure p_procedure);
void vtcp_uninitialize(struct vtcp *pvtcp);

int vtcp_session_close(struct vtcp *pvtcp, const void *psession);

struct vtcp_door *vtcp_search_door(struct vtcp *pvtcp, const unsigned char *address, unsigned int addresssize);

struct vtcp_door *vtcp_door_open(struct vtcp *pvtcp, void *pointer, const unsigned char *address, unsigned int addresssize);
int vtcp_door_close(struct vtcp *pvtcp, struct vtcp_door *pdoor);

#endif