// declare globally shared array
extern int arr_q[5];

// declare globally shared semaphores
extern sid32 writeSem;
extern sid32 readSem;
extern sid32 mutex;

// declare globally shared read and write indices
extern int currWrite;
extern int currRead;

// function prototypes
void consumer_bb(int id, int count);
void producer_bb(int id, int count);