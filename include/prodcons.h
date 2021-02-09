/*Global variable for producer consumer*/
extern int n;

extern sid32 can_write;
extern sid32 can_read;
extern sid32 mutex;

/*function Prototype*/
void consumer(int count);
void producer(int count);