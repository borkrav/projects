//RNG code taken from here
//http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
//it's probably good enough for now, enhance later if I need to

uint rng_state;


uint rand_xorshift(){
	rng_state ^= (rng_state << 13);
    rng_state ^= (rng_state >> 17);
    rng_state ^= (rng_state << 5);
    return rng_state;
}

uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}