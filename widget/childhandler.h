
#ifdef MSWIN

#define childhandler_()  do { } while(0)

#else

void set_child_handler (void);
void CChildWait (pid_t p);
int CChildExitted (pid_t p, int *status);
int CChildCheckExitted (pid_t p);
void childhandler_ (void);

#endif




