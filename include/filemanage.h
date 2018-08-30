
#ifndef __FILE_MANAGE_H_

#define __FILE_MANAGE_H_

#include <time.h>

#define   DISPLAY_MAX_NUM								 10   /*每次搜索文件的个数*/

/*存储文件名的结构体*/
typedef struct tag_rec_dirent
{
	unsigned short	d_ino; 		/*条目编号*/
	unsigned short	lock;
	unsigned int 	start_time;  /*文件开始时间*/
	unsigned int 	end_time;	/* 文件结束时间*/
	unsigned int    filesize;     /*文件大小以K为单位*/
	unsigned int	channel; 	/* 通道号*/
	int				m_filetype;
	int				m_alarmtype;
	char			d_name[96]; 		/*文件名(有路径)*/
}rec_dirent;

typedef struct 
{
	int	fileNb;//文件个数
	rec_dirent    namelist[DISPLAY_MAX_NUM];
}RecordFileName;

typedef struct TagFindFileType
{
	struct  tm time;
	int		devType;//2/*设备类型0-HDD 1-MHDD 2-SD*/
	int 	channel;//2/*通道1~16*/
	int		RecordType;//2/*录像类型0-全部，1-定时，2-手动，3-报警*/
	int		SearchType;/*查询方式 0 ，录像类型查询，1 司机名查询 2   车牌号查询*/
	char	m_drivername[17];
	char	m_vehiclenum[17];	
}FindFileType;

#endif 
