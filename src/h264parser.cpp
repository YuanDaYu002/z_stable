#include "h264parser.h"
#include "aes.h"


UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	//计算0bit的个数
	UINT nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit ++;


	//计算结果
	DWORD dwRet = 0;
	for (UINT i=0; i<nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}


int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	int UeVal=Ue(pBuff,nLen,nStartBit);
	double k=UeVal;
	int nValue=ceil(k/2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2==0)
		nValue=-nValue;
	return nValue;
}


DWORD u(UINT BitCount,BYTE * buf,UINT &nStartBit)
{
	DWORD dwRet = 0;
	for (UINT i=0; i<BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return dwRet;
}

/**
 * H264的NAL起始码防竞争机制 
 *
 * @param buf SPS数据内容
 *
 * @无返回值
 */ 
void de_emulation_prevention(BYTE* buf,unsigned int* buf_size)
{
	int i=0,j=0;
	BYTE* tmp_ptr=NULL;
	unsigned int tmp_buf_size=0;
	int val=0;

	tmp_ptr=buf;
	tmp_buf_size=*buf_size;
	for(i=0;i<(tmp_buf_size-2);i++)
	{
		//check for 0x000003
		val=(tmp_ptr[i]^0x00) +(tmp_ptr[i+1]^0x00)+(tmp_ptr[i+2]^0x03);
		if(val==0)
		{
			//kick out 0x03
			for(j=i+2;j<tmp_buf_size-1;j++)
				tmp_ptr[j]=tmp_ptr[j+1];

			//and so we should devrease bufsize
			(*buf_size)--;
		}
	}

	return;
}

/**
 * 解码SPS,获取视频图像宽、高信息 
 *
 * @param buf SPS数据内容
 * @param nLen SPS数据的长度
 * @param width 图像宽度
 * @param height 图像高度

 * @成功则返回1 , 失败则返回0
 */ 
int h264_decode_sps(BYTE * buf,unsigned int nLen,int &width,int &height, int &fps)
{
	UINT StartBit=0; 
	fps=0;
	de_emulation_prevention(buf,&nLen);

	int forbidden_zero_bit=u(1,buf,StartBit);
	int nal_ref_idc=u(2,buf,StartBit);
	int nal_unit_type=u(5,buf,StartBit);
	if(nal_unit_type==7)
	{
		int profile_idc=u(8,buf,StartBit);
		int constraint_set0_flag=u(1,buf,StartBit);//(buf[1] & 0x80)>>7;
		int constraint_set1_flag=u(1,buf,StartBit);//(buf[1] & 0x40)>>6;
		int constraint_set2_flag=u(1,buf,StartBit);//(buf[1] & 0x20)>>5;
		int constraint_set3_flag=u(1,buf,StartBit);//(buf[1] & 0x10)>>4;
		int reserved_zero_4bits=u(4,buf,StartBit);
		int level_idc=u(8,buf,StartBit);

		int seq_parameter_set_id=Ue(buf,nLen,StartBit);

		if( profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144 )
		{
			int chroma_format_idc=Ue(buf,nLen,StartBit);
			if( chroma_format_idc == 3 )
				int residual_colour_transform_flag=u(1,buf,StartBit);
			int bit_depth_luma_minus8=Ue(buf,nLen,StartBit);
			int bit_depth_chroma_minus8=Ue(buf,nLen,StartBit);
			int qpprime_y_zero_transform_bypass_flag=u(1,buf,StartBit);
			int seq_scaling_matrix_present_flag=u(1,buf,StartBit);

			int seq_scaling_list_present_flag[8];
			if( seq_scaling_matrix_present_flag )
			{
				for( int i = 0; i < 8; i++ ) {
					seq_scaling_list_present_flag[i]=u(1,buf,StartBit);
				}
			}
		}
		int log2_max_frame_num_minus4=Ue(buf,nLen,StartBit);
		int pic_order_cnt_type=Ue(buf,nLen,StartBit);
		if( pic_order_cnt_type == 0 )
			int log2_max_pic_order_cnt_lsb_minus4=Ue(buf,nLen,StartBit);
		else if( pic_order_cnt_type == 1 )
		{
			int delta_pic_order_always_zero_flag=u(1,buf,StartBit);
			int offset_for_non_ref_pic=Se(buf,nLen,StartBit);
			int offset_for_top_to_bottom_field=Se(buf,nLen,StartBit);
			int num_ref_frames_in_pic_order_cnt_cycle=Ue(buf,nLen,StartBit);

			int *offset_for_ref_frame=new int[num_ref_frames_in_pic_order_cnt_cycle];
			for( int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
				offset_for_ref_frame[i]=Se(buf,nLen,StartBit);
			delete [] offset_for_ref_frame;
		}
		int num_ref_frames=Ue(buf,nLen,StartBit);
		int gaps_in_frame_num_value_allowed_flag=u(1,buf,StartBit);
		int pic_width_in_mbs_minus1=Ue(buf,nLen,StartBit);
		int pic_height_in_map_units_minus1=Ue(buf,nLen,StartBit);
		
		width=(pic_width_in_mbs_minus1+1)*16;
		height=(pic_height_in_map_units_minus1+1)*16;

		int frame_mbs_only_flag=u(1,buf,StartBit);
		if(!frame_mbs_only_flag)
			int mb_adaptive_frame_field_flag=u(1,buf,StartBit);

		int direct_8x8_inference_flag=u(1,buf,StartBit);
		int frame_cropping_flag=u(1,buf,StartBit);
		if(frame_cropping_flag)
		{
			int frame_crop_left_offset=Ue(buf,nLen,StartBit);
			int frame_crop_right_offset=Ue(buf,nLen,StartBit);
			int frame_crop_top_offset=Ue(buf,nLen,StartBit);
			int frame_crop_bottom_offset=Ue(buf,nLen,StartBit);
		}
		int vui_parameter_present_flag=u(1,buf,StartBit);
		if(vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag=u(1,buf,StartBit);              
			if(aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc=u(8,buf,StartBit);   
				if(aspect_ratio_idc==255)
				{
					int sar_width=u(16,buf,StartBit);                                  
					int sar_height=u(16,buf,StartBit);                                      
				}
			}
			int overscan_info_present_flag=u(1,buf,StartBit); 
			if(overscan_info_present_flag)
				int overscan_appropriate_flagu=u(1,buf,StartBit);                   
			int video_signal_type_present_flag=u(1,buf,StartBit); 
			if(video_signal_type_present_flag)
			{
				int video_format=u(3,buf,StartBit);                         
				int video_full_range_flag=u(1,buf,StartBit);                       
				int colour_description_present_flag=u(1,buf,StartBit);
				if(colour_description_present_flag)
				{
					int colour_primaries=u(8,buf,StartBit);              
					int transfer_characteristics=u(8,buf,StartBit);                     
					int matrix_coefficients=u(8,buf,StartBit);                  		
				}
			}
			int chroma_loc_info_present_flag=u(1,buf,StartBit);  
			if(chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field=Ue(buf,nLen,StartBit);             
				int chroma_sample_loc_type_bottom_field=Ue(buf,nLen,StartBit);       
			}
			int timing_info_present_flag=u(1,buf,StartBit);        
			if(timing_info_present_flag)
			{
				int num_units_in_tick=u(32,buf,StartBit);                              
				int time_scale=u(32,buf,StartBit);    
				fps=time_scale/(2*num_units_in_tick);
			}
		}
		return true;
	}
	else
		return false;
}



static int FindStartCode2(unsigned char *Buf) {
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 1) return 0; //0x000001?  
    else return 1;
}

static int FindStartCode3(unsigned char *Buf) {
    if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 0 || Buf[3] != 1) return 0;//0x00000001?  
    else return 1;
}

int get_data_from_buffer(unsigned char *buf,int size,void *data)
{
	buffer_t *b = (buffer_t*)data;
	if(size==0)
	{
		return -1;
	}
	else if (size<0)
	{
		if(b->offset+size<0)
			return -1;
		b->offset+=size;
		return 0;
	}
	else
	{
		if(b->offset==b->size)
		{
			return 0;
		}
		if(b->offset+size<=b->size)
		{
			memcpy(buf,b->buf+b->offset,size);
			b->offset+=size;
			return size;
		}
		else if(b->offset+size>b->size)
		{
			int ret=b->size-b->offset;
			memcpy(buf,b->buf+b->offset,ret);
			b->offset=b->size;
			return ret;
		}
		
	}
	return -1;

}
int get_data_from_file(unsigned char *buf,int size,void *data)
{
	FILE *file = (FILE *)data;
	if(size==0)
	{
		return -1;
	}
	else if (size<0)
	{
		
		if(0!= fseek(file, size, SEEK_CUR))
			return -1;
		return 0;
	}
	else
	{
		return fread(buf, 1, size, file);
	}
	return -1;
}

int GetH264NALU(NALU_t *nalu,void *usr_data,get_data_cb get) {
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf;

    if ((Buf = (unsigned char*)calloc(nalu->max_size, sizeof(char))) == NULL)
    {
	  	printf("GetAnnexbNALU: Could not allocate Buf memory\n");
		return -1;
    }
    nalu->startcodeprefix_len = 3;


	if (3 != get(Buf, 3, usr_data)) {
			free(Buf);
			return 0;
		}	
    nalu->info2 = FindStartCode2(Buf);
    if ( nalu->info2 != 1) {
        if (1 != get(Buf + 3, 1, usr_data)) {
            free(Buf);
            return 0;
        }
         nalu->info3 = FindStartCode3(Buf);
        if ( nalu->info3 != 1) {
            free(Buf);
            return -1;
        }
        else {
            pos = 4;
            nalu->startcodeprefix_len = 4;
        }
    }
    else {
        nalu->startcodeprefix_len = 3;
        pos = 3;
    }
    StartCodeFound = 0;
    nalu->info2 = 0;
    nalu->info3 = 0;

    while (!StartCodeFound) 
	{
        if (get(&Buf[pos], 1, usr_data)==0) 
		{
			pos++;
            nalu->len = (pos - 1) - nalu->startcodeprefix_len;
            memcpy(nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
            nalu->forbidden_bit = nalu->buf[0] & 0x80;// //1 bit  
            nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit  
            nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit  
            free(Buf);
            return pos - 1;
        }		
		pos++;
        //Buf[pos++] = fgetc(h264bitstream);
        nalu->info3 = FindStartCode3(&Buf[pos - 4]);
        if (nalu->info3 != 1)
            nalu->info2 = FindStartCode2(&Buf[pos - 3]);
        StartCodeFound = (nalu->info2 == 1 || nalu->info3 == 1);
    }

    // Here, we have found another start code (and read length of startcode bytes more than we should  
    // have.  Hence, go back in the file  
    rewind = (nalu->info3 == 1) ? -4 : -3;

    if (0 != get(NULL, rewind, usr_data)) {
        free(Buf);
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
		return 0;
    }

    // Here the Start code, the complete NALU, and the next start code is in the Buf.    
    // The size of Buf is pos, pos+rewind are the number of bytes excluding the next  
    // start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code  

    nalu->len = (pos + rewind) - nalu->startcodeprefix_len;
    memcpy(nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//  
    nalu->forbidden_bit = nalu->buf[0] & 0x80;/// //1 bit  
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit  
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit  
    free(Buf);

    return (pos + rewind);
}

int GetMetaNALU(metavideo_t *metaData,NALU_t *n,void *usr_data,get_data_cb get) 
{
	if(metaData==NULL)
		return -1;
	memset(metaData,0,sizeof(metavideo_t));
	
	printf("++++++++++++++\n");
	while(1)
	{
		if(GetH264NALU(n,usr_data,get)<=0)
			break;
		metaData->idr_offset+=n->len+4;
		printf("find meta NALU size:%8d,%d\n",n->len,n->nal_unit_type);
		if(n->nal_unit_type == NALU_TYPE_SPS&&metaData->Sps==NULL)
		{
			//NaluUnit naluUnit;	
			// 读取SPS帧	 
			metaData->nSpsLen = n->len;	  
			metaData->Sps=NULL;	
			metaData->Sps=(unsigned char*)malloc(n->len);  
			memcpy(metaData->Sps,n->buf,n->len); 
			h264_decode_sps(metaData->Sps,metaData->nSpsLen,metaData->nWidth,metaData->nHeight,metaData->nFrameRate); 
			printf("sps----------------------OK!!! [%d,%d,%d]\n",metaData->nWidth,metaData->nHeight,metaData->nFrameRate);
		}	    // 读取PPS帧		
		if(n->nal_unit_type == NALU_TYPE_PPS&&metaData->Pps==NULL)
		{
		      
		    metaData->nPpsLen = n->len;   
		    metaData->Pps=NULL;  
		    metaData->Pps=(unsigned char*)malloc(n->len);  
		    memcpy(metaData->Pps,n->buf,n->len);
			printf("sps----------------------OK!!!\n");
			//break;
		}
		if(n->nal_unit_type == NALU_TYPE_IDR)
		{		
			metaData->idr_offset-=n->len;
			printf("IDR----------------------[%d]!!!\n",metaData->idr_offset);
			break;
		}		
	}
	if(metaData->nFrameRate<=0||metaData->nFrameRate>=30)
		metaData->nFrameRate=10;

	printf("++++++++++++++\n");


	return 0;
}
int Parsr_H264_Nalu_List(char *url,int max,NALU_t **nalu,metavideo_t *meta)
{

	NALU_t *n;
	int buffersize = 1024*100;

	//FILE *myout=fopen("output_log.txt","wb+");  
	//FILE *myout = stdout;

	FILE *h264bitstream = fopen(url, "rb+");
	if (h264bitstream == NULL) {
	    perror("Open file error\n");
	    return 0;
	}

	n = (NALU_t*)calloc(1, sizeof(NALU_t));
	if (n == NULL) {
	    printf("Alloc NALU Error\n");
		fclose(h264bitstream);
	    return 0;
	}

	n->max_size = buffersize;
	n->buf = (char*)calloc(buffersize, sizeof(char));
	if (n->buf == NULL) {
	    free(n);
	    printf("AllocNALU: n->buf");
		fclose(h264bitstream);
	    return 0;
	}
	NALU_t *p = (NALU_t*)calloc(1, sizeof(NALU_t)*max);
	*nalu=p;
	
	GetMetaNALU(meta,n,(void*)h264bitstream,get_data_from_file);
	

	
	int nalucount=0;
	
	while (1)
	{
	    
	   	if( GetH264NALU(n,(void*)h264bitstream,get_data_from_file)<=0)
			break;
		if(n->nal_unit_type == 0x06||n->nal_unit_type == 0x07 || n->nal_unit_type == 0x08)	
				continue;	
		p->max_size =  n->len;
		
		p->startcodeprefix_len =  n->startcodeprefix_len;
		p->len =  n->len;
		p->max_size =  n->len;
		p->forbidden_bit =  n->forbidden_bit;

		p->nal_reference_idc =	n->nal_reference_idc;
		p->nal_unit_type =  n->nal_unit_type;		
		p->buf = (char*)calloc( p->max_size, sizeof(char));
		memcpy(p->buf, n->buf,n->len);
		p++;
		nalucount++;

		
	}
	printf("Parsr_H264_Nalu_List--->w:%d,h:%d,fps:%d,nalucount:%d\n",meta->nWidth,meta->nHeight,meta->nFrameRate,nalucount);
	fclose(h264bitstream);
	free(n->buf);
	free(n);
	return nalucount;
}
int parser_video_meta(unsigned char *data,unsigned int size,metavideo_t *meta)
{
	NALU_t nalu;
	memset(&nalu,0,sizeof(NALU_t));
	nalu.max_size = size;
	nalu.buf = (char*)calloc(size, sizeof(char));
	buffer_t buffer;
	memset(&buffer,0,sizeof(buffer_t));
	buffer.buf=(char*)data;
	buffer.offset=0;
	buffer.size=size;
	GetMetaNALU(meta,&nalu,(void*)(&buffer),get_data_from_buffer);
	meta->isparser = 1;
	free(nalu.buf);
	return 0;
}



int aes_encrypt(unsigned char *input, unsigned char *output)
{ 
	mbedtls_aes_context ctx; 
	int ret; 
	unsigned char key_str[100]={0}; 
	unsigned char iv_str[100]={0}; 
	//strcpy((char*)key_str,(const char*)GetAesKey()); 

	mbedtls_aes_init( &ctx ); 

	mbedtls_aes_setkey_enc( &ctx, key_str, 256 ); 
	ret = mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, 256, iv_str, input, output); 
	//printf("enc ret %d\n",ret); 
	mbedtls_aes_free( &ctx ); 
	return 0; 
}

int aes_decrypt(const char *aes_key,unsigned char *input, unsigned char *output)
{
	mbedtls_aes_context ctx;	
	int ret;
	
	unsigned char key_str[100]={0};
    unsigned char iv_str[100]={0};
	strcpy((char*)key_str,aes_key);
	mbedtls_aes_init( &ctx );
	mbedtls_aes_setkey_dec( &ctx, key_str, 256 );
    mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, 256, iv_str, input, output);
	mbedtls_aes_free( &ctx );
	return 0;
}





