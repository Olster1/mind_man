struct bucket
{
    u64 OccupiedBitField;
    void *E;
};

struct bucket_list
{
    memory_arena *Arena;
    u32 BucketSize;
    u32 ElementSize;
    u64 OccupiedBitField;
    
    // TODO(OLIVER): Turn this into a linked list to have unlimited space...
    bucket Buckets[64];
    u32 BucketCount;
    
    inline u32
        Add(void *SourceData)
    {
        s32 ResultIndex = -1;
        b32 Found = false;
        u32 BucketIndex = 0;
        
        for(u32 BucketIndex = 0; BucketIndex < ArrayCount(Buckets) && !Found; ++BucketIndex)
        {
            bucket *Bucket = Buckets + BucketIndex;
            
            if(!Bucket->E)
            {
                Bucket->E = PushSize(Arena, BucketSize*ElementSize, true);
                Bucket->OccupiedBitField = 0;
            }
            
            u32 BucketOccupiedFlag = (1 << BucketIndex);
            b32 BucketOccupied = OccupiedBitField & BucketOccupiedFlag;
            if(!BucketOccupied)
            {
                forN(BucketSize)
                {
                    
                    u32 ElementOccupiedFlag = (1 << BucketSizeIndex);
                    b32 ElementOccupied = ElementOccupiedFlag & Bucket->OccupiedBitField;
                    
                    if(!ElementOccupied)
                    {
                        u8 *BucketData = (u8 *)Bucket->E + BucketSizeIndex*ElementSize;
                        Bucket->OccupiedBitField |= ElementOccupiedFlag;
                        ResultIndex = BucketIndex + BucketSizeIndex;
                        
                        MemoryCopy(SourceData, BucketData, ElementSize);
                        
                        if((BucketSizeIndex + 1) == BucketSize ||
                           (1 << (BucketSizeIndex + 1)) & Bucket->OccupiedBitField)
                        {
                            OccupiedBitField |= BucketOccupiedFlag;
                        }
                        
                        Found = true;
                        break;
                    }
                }
            }
        }
        
        Assert(Found && ResultIndex != -1);
        
        return ResultIndex;
        
    }
    
    inline void *
        Get(u32 Index)
    {
        void *Result = 0;
        u32 BucketIndex = Index / BucketSize;
        
        Assert(BucketIndex < ArrayCount(Buckets));
        
        bucket *Bucket = Buckets + BucketIndex;
        
        if(Bucket->E)
        {
            u32 ElementIndex = Index - BucketIndex;
            Assert(!(Bucket->OccupiedBitField & (1 << ElementIndex)));
            Result = (u8 *)Bucket->E + ElementIndex*ElementSize;
        }
        
        return Result;
    }
    
    inline void 
        Remove(u32 Index)
    {
        u32 BucketIndex = Index / BucketSize;
        bucket *Bucket = Buckets + BucketIndex;
        
        if(Bucket->E)
        {
            u32 ElementIndex = Index - BucketIndex;
            Bucket->OccupiedBitField &= ~(1 << ElementIndex);
            
            OccupiedBitField &= ~(1 << BucketIndex);
        }
    }
};

#define InitBucketList(Arena, Count, Type) InitBucketList_(Arena, Count, sizeof(Type))

inline bucket_list
InitBucketList_(memory_arena *Arena, u32 BucketSize, u32 SizeOfType)
{
    bucket_list Result = {};
    
    Assert(BucketSize < 64);
    
    Result.Arena = Arena;
    Result.BucketSize = BucketSize;
    Result.ElementSize = SizeOfType;
    Result.OccupiedBitField = 0;
    Result.BucketCount = 0;
    
    return Result;
}
