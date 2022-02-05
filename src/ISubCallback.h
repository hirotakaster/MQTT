
#ifndef __ISUBCALLBACK_
#define __ISUBCALLBACK_

class ISubCallback
{
public:
  virtual void Callback(char*, uint8_t*, unsigned int) = 0;
};


#endif
