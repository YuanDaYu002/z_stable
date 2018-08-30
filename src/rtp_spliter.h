#ifndef RTPSPLITER_H
#define RTPSPLITER_H

class RTPSpliter
{
 public:
   RTPSpliter();
   ~RTPSpliter();
   void SplitFrame(int cmd_type, const char* buffer, int size, int frame_flag, int chlnum);
   const char* GetSendBuffer();
   int  GetSendBufferSize();

 private:
   class SendBuffer
   {
     public:
         SendBuffer();
         ~SendBuffer();
		 //before use , set buffer size first
         void  SetBufferSize(int size);
         char* GetBuffer();
         int   GetBufferSize();

         //write data at the tail of buffer
         //return filled size
         int   PushBack(const char* buffer, int bsize);
         // get the bytes that wrote to the buffer
         int   GetFilledSize();

         //empty buffer
         void  ResetBuffer();

     private:
         char* m_buffer;
         int   m_bufferSize;
         int   m_filledSize;
   };
   SendBuffer m_buffer;
};


#endif /* end of include guard: RTPSPLITER_H */

