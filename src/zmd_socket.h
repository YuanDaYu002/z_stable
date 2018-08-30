#ifndef ZMDSOCKET_H
#define ZMDSOCKET_H

class ZmdSocket
{
 public:
   virtual void SetHandle(int sock)=0;
   virtual bool IfReadable(int ms)=0;
   virtual bool IfWritable(int ms)=0;
   virtual int  Read(void* data, int len)=0;
   virtual int  ReadN(void* data, int len)=0;
   virtual int  WriteN(const void* data, int len)=0;
   virtual int  Write(const void* data, int len)=0;
   virtual void Close()=0;
   virtual int  Select(bool &read, bool &write, int ms)=0;
   virtual int  SetBlocking(bool blocking)=0;

};
#endif /* end of include guard: ZMDSOCKET_H */
