inline int qAllocMore(int alloc, int extra)
{
    const int page = 1<<12;
    int nalloc;
    alloc += extra;
    if (alloc < 1<<6) {
	nalloc = (1<<3) + ((alloc >>3) << 3);
    } else if (alloc < page) {
	nalloc = 1<<3;
	while (nalloc < alloc)
	    nalloc *= 2;
    } else {
	nalloc = ((alloc + page) / page) * page;
    }
    return nalloc - extra;
}


