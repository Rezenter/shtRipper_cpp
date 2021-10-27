#include "magn_file.h"

union float2char{
    float num;
    char ch[4];
};

bool ImportNIIFAFile(const char *FileName)
{
    DWORD NBytes;
    HANDLE hFile;
    DWORD Size;
    int len;
    char *buff,*p1,*p2,*pStart;
    char XName[16];
    YNAME *YName;
    double **YData,*XData;
    int i,j,Count;
    //HISTOGRAM *H;
    hFile=CreateFile(FileName,
                     GENERIC_READ,
                     0,
                     nullptr,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     nullptr);
    if (hFile==INVALID_HANDLE_VALUE)
        return false;
    Size=GetFileSize(hFile,nullptr);
    buff=(char *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,Size);
    ReadFile(hFile,buff,Size,&NBytes,nullptr);
    CloseHandle(hFile);

    len=0;
    p1=strstr(buff,"t_ms");
    if (p1==nullptr)
    {
        p1=strstr(buff+4,"t_ms");
    }
    p2=strstr(p1," ");
    char tmp = p2[0];
    p2[0]=0;
    strcpy(XName,p1);
    p2[0] = tmp;
    len+=strlen(XName)+1;
    pStart=p1=p2+1;
    Count=0;
    do
    {
        p2=strpbrk(p1," \n");
        Count++;
        if (p2[0]=='\n')
            break;
        p1=p2+1;
    }
    while (true);
    //len=(DWORD)(p2+1)-(DWORD)buff;
    len=(p2+1)-buff;
    len=(Size-len)/4/(Count+1);
    YName=(YNAME *)GlobalAlloc(GMEM_FIXED,sizeof(YNAME)*Count);
    XData=(double *)GlobalAlloc(GMEM_FIXED,sizeof(double)*len);
    YData=(double **)GlobalAlloc(GMEM_FIXED,sizeof(double *)*Count);
    p1=pStart;
    for (i=0;i<Count;i++)
    {
        p2=strpbrk(p1," \r\n");
        tmp = p2[0];
        p2[0]=0;
        strcpy(YName[i],p1);
        p2[0] = tmp;
        YData[i]=(double *)GlobalAlloc(GMEM_FIXED,sizeof(double)*len);
        p1=p2+1;
    }

    float2char converter{};

    for (i=0;i<len;i++)
    {
        XData[i]=*((float *)p1);
        p1+=4;
        for (j=0;j<Count;j++)
        {
            YData[j][i]=*((float *)p1);

            if(std::string(YName[j]) == "Itf"){
                converter.num = *((float *)p1) * 1.5;
                for(char ch_val : converter.ch){
                    *p1 = ch_val;
                    p1 += 1;
                }
            }else{
                p1+=4;
            }
        }
    }
    for (i=0;i<Count;i++)
    {
        std::cout << YName[i] << std::endl;
        //H=CreateHist(YData[i],len,0);
        //strcpy(H->Name,YName[i]);
        //H->Tmin=XData[0]/1000;
        //H->Tmax=XData[len-1]/1000;
        //AddHist(H,0);
        GlobalFree(YData[i]);
    }
    GlobalFree(YName);
    GlobalFree(XData);
    GlobalFree(YData);

    std::string outFile = "d:/data/cfm/mod/00040032_x1.5.dat";
    hFile=CreateFile(outFile.c_str(),
                     GENERIC_WRITE,
                     0,
                     nullptr,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     nullptr);
    if (hFile==INVALID_HANDLE_VALUE) {
        std::cout << "Failed to open output file" << std::endl;
        std::cout << GetLastError() << std::endl;
        return false;
    }
    WriteFile(hFile, buff, Size, &NBytes, nullptr);
    std::cout << "written: " << NBytes << std::endl;
    CloseHandle(hFile);

    GlobalFree(buff);
    return true;
}