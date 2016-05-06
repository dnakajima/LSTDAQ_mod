void inverseByteOrder(char *buf,int bufsize);
void test()
{

  unsigned char *p;
  unsigned char headerbuf[16];
  p=headerbuf;
  
  memcpy(p,"123499999999",12);
  p+=12;
  FILE *fp_data;
  if((fp_data = fopen("hogehoge","w"))==NULL){
    cout<<"output file open error!!"<<endl;
    exit(1);
  }

  unsigned long cc=0;
  for(unsigned int i=0;i<1000;i++)
    {
      cc=(unsigned long)i;
      //      unsigned char tempcc[8];
      
      // unsigned int *b_p_ca;
      // unsigned long b_ca;
      // b_p_ca=(unsigned int*)&cc;
      // b_ca=(unsigned long )*b_p_ca;
      // cout<<b_ca<<endl;
      // inverseByteOrder((char *)&b_ca,sizeof(b_ca));

      
      // memcpy(p,&b_ca,sizeof(b_ca));
      memcpy(p,&cc,sizeof(cc));
      //headerbuf[4]=i%265;
      memcpy(&headerbuf[4],&i,sizeof(i));
      fwrite(headerbuf,16,1,fp_data);
      
      unsigned int *p_ca;
      unsigned long ca;

      p_ca=(unsigned int*)&headerbuf[12];
      ca=(unsigned long)*p_ca;
      //inverseByteOrder((char *)&ca,sizeof(ca));
      cout<<ca<<endl;
    }
  fclose(fp_data);

}
void inverseByteOrder(char *buf,int bufsize)
{
  // if(bufsize>32)exit 1;
  unsigned char tempbuf[32];
  for(int i=0; i<bufsize; i++)
  {
    tempbuf[i]=buf[bufsize-i-1];
  }
  memcpy(buf,tempbuf,bufsize);
}
