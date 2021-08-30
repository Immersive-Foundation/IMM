//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piTypes.h"

namespace ImmCore {

// 3/4
class piString
{
public:
    piString();
    ~piString();

    piString(piString &&) = delete;
    piString(const piString &) = delete;
    piString& operator=(piString const&) = delete;
    piString& operator=(piString &&); // we allow MOVE, but only for initialized strings
                                      // not as means of constructing/initializing. For 
                                      // that, use InitMove()

	bool        Init(uint32_t max);
	bool        InitCopy( const piString *ori);
	bool        InitCopyW(const wchar_t *ori);
	bool        InitCopyS(const char *ori);
	bool        InitCopyS(const char *ori, uint32_t len);
	bool        InitConcatW(const wchar_t *ori1, const wchar_t *ori2);
	bool        InitConcatW(const wchar_t *ori1, const wchar_t *ori2, const wchar_t *ori3);
	bool        InitWrapW(const wchar_t *ori);
    void        InitMove( piString *ori);
	bool        Copy(const piString *ori);
	bool        CopyW(const wchar_t *ori);
	bool        CopyS(const char *ori);
    bool        ConcatW(const wchar_t *ori1, const wchar_t *ori2);
    bool        ConcatW(const wchar_t *ori1, const wchar_t *ori2, const wchar_t *ori3);
    bool        IsNull(void) const;
    void        SetNull(void);
    void        End( void );
    int         GetLength( void ) const;
    void        SetLength(uint32_t len);
    void        Reset( void );

    bool        AppendWC( wchar_t obj);
    bool        AppendAC( char obj);

    bool		AppendCAt( wchar_t obj, uint32_t pos);
    bool        AppendS( const piString *str);
    bool        AppendSW( const wchar_t *str);
    bool        AppendSA( const char *str);
    bool        AppendPrintf( const wchar_t *format, ...);
    bool		RemoveCAt(uint32_t pos);

	bool           Equal( const piString *str) const;
	bool           EqualW( const wchar_t *str) const;
    bool           ContainsW( const wchar_t *str) const;
    const wchar_t  GetC(uint32_t n) const;
    const wchar_t *GetS(void) const;
    bool           Printf( const wchar_t *format, ...);
    bool           ToSInt(  int *res) const;
    bool           ToUInt(uint32_t *res) const;
    bool           ToFloat(  float *res) const;
	void           ToUpper(void);

    bool           RemoveOccurrences(const wchar_t what);
    bool           ReplaceOccurrences(const wchar_t what, const wchar_t bywhat);
private:
    bool          iReallocate(void);
    bool          iInit(uint32_t max);

    wchar_t      *mBuffer;		// 1/2
    uint32_t      mMax;			// 1
    uint32_t      mNum;		    // 1
    #ifdef _DEBUG
    uint32_t       mInited;
    static const uint32_t UNINITED_MARK;
    static const uint32_t INITED_MARK;
#endif
};

} // namespace ImmCore

