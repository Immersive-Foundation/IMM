//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

namespace ImmCore {

class piGL4X_RenderContext
{
public:
	piGL4X_RenderContext() {}
	~piGL4X_RenderContext() {}

public:
    int  Create( const void **hwnd, int num, bool disableVSynch, bool doublebuffered, bool antialias, bool disableErrors);
	bool CreateFromCurrent(void);
    int  SetActiveWindow( int id );
    void Destroy( void );
    void Enable( void );
    void Disable( bool doSwapBuffers );
    void SwapBuffers( void );
    void Delete( void );
public:
    void *mData;
};

} // namespace ImmCore
