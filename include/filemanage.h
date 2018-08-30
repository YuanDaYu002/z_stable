
#ifndef __FILE_MANAGE_H_

#define __FILE_MANAGE_H_

#include <time.h>

#define   DISPLAY_MAX_NUM								 10   /*ÿ�������ļ��ĸ���*/

/*�洢�ļ����Ľṹ��*/
typedef struct tag_rec_dirent
{
	unsigned short	d_ino; 		/*��Ŀ���*/
	unsigned short	lock;
	unsigned int 	start_time;  /*�ļ���ʼʱ��*/
	unsigned int 	end_time;	/* �ļ�����ʱ��*/
	unsigned int    filesize;     /*�ļ���С��KΪ��λ*/
	unsigned int	channel; 	/* ͨ����*/
	int				m_filetype;
	int				m_alarmtype;
	char			d_name[96]; 		/*�ļ���(��·��)*/
}rec_dirent;

typedef struct 
{
	int	fileNb;//�ļ�����
	rec_dirent    namelist[DISPLAY_MAX_NUM];
}RecordFileName;

typedef struct TagFindFileType
{
	struct  tm time;
	int		devType;//2/*�豸����0-HDD 1-MHDD 2-SD*/
	int 	channel;//2/*ͨ��1~16*/
	int		RecordType;//2/*¼������0-ȫ����1-��ʱ��2-�ֶ���3-����*/
	int		SearchType;/*��ѯ��ʽ 0 ��¼�����Ͳ�ѯ��1 ˾������ѯ 2   ���ƺŲ�ѯ*/
	char	m_drivername[17];
	char	m_vehiclenum[17];	
}FindFileType;

#endif 
