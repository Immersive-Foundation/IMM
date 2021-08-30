//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
namespace ImmCore {

void piAssert(bool e)
{
    if (!e)  __debugbreak();
}


}
