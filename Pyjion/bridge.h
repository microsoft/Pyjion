extern int pyjit_log(const char *__restrict format, ...);

#ifdef PLATFORM_UNIX

extern "C" void *mmap(void *addr, size_t length, int prot, int flags,
	int fd, size_t offset); 

extern "C" int munmap(void *addr, size_t length);

extern "C" int g_executableMmapProt;
extern "C" int g_privateAnonMmapFlags;

#endif


extern "C" size_t pyjit_pagesize();

