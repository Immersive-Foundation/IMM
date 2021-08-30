#include <malloc.h>
#include <algorithm>
#include <string>

//#include "libImmCore/src/libMisc/formats/piJSONReader.h"
#include "simpleJSON/JSON.h"
#include "libImmCore/src/libBasics/piFile.h"

#include "settings.h"

using namespace ImmCore;
namespace ExePlayer
{
    Settings::Settings() {}

    Settings::~Settings() {}

    static ImmCore::trans3d iDeserializeTransform(const JSONValue* value)
    {
        JSONObject jo = value->AsObject();
        JSONArray rot = jo[L"Rotation"]->AsArray();
        JSONArray pos = jo[L"Translation"]->AsArray();
        const double scale = jo[L"Scale"]->AsNumber();
        const wchar_t* fl = jo[L"Flip"]->AsString().c_str();

        const quatd rotation = quatd(rot[0]->AsNumber(), rot[1]->AsNumber(), rot[2]->AsNumber(), rot[3]->AsNumber());
        const vec3d translation = vec3d(pos[0]->AsNumber(), pos[1]->AsNumber(), pos[2]->AsNumber());
        const flip3 flip = (fl[0] == L'X') ? flip3::X :
            (fl[0] == L'Y') ? flip3::Y :
            (fl[0] == L'Z') ? flip3::Z :
            flip3::N;
        return ImmCore::trans3d(rotation, scale, flip, translation);
    }

    void Settings::End(void)
    {
        for (ImmCore::piString& load : mFiles.mLoad)
        {
            load.End();
        }
        mFiles.mLoad.End();
        mPlayback.mPlayerSpawn.mLocation.End();
        mSound.mDevice.End();
    }

    bool Settings::Init(const wchar_t* filename, ImmCore::piLog* log)
    {
        // read json file as raw text
        piFile fp;
        if( !fp.Open(filename, L"rb") )
        {
            log->Printf(LT_ERROR, L"Could not read config file");
            return false;
        }
        const int len = (int)fp.GetLength();
	    char *json = (char*)malloc( len+1 );
	    if( !json ) { fp.Close(); return false; };
        fp.Read( (void*)json, len);
	    json[len] = 0;
	    fp.Close();

        // parse json
        JSONValue* jvRoot = JSON::Parse(json);
        if (jvRoot == nullptr)
        {
            log->Printf(LT_ERROR, L"Could not parse config file");
            return false;
        }
        if (!jvRoot->IsObject())
        {
            log->Printf(LT_ERROR, L"Could not parse config file");
            return false;
        }

        if (!mFiles.mLoad.Init(1024, false))
            return false;

        JSONObject joRoot = jvRoot->AsObject();
        // file
        {
            JSONObject joFile = joRoot[L"File"]->AsObject();
            JSONArray jLoadFiles = joFile[L"Load"]->AsArray();
            for (int i = 0; i < jLoadFiles.size(); i++)
            {
                ImmCore::piString* fileToLoad = mFiles.mLoad.New(1, true);
                piAssert(fileToLoad != nullptr);
                if (!fileToLoad->InitCopyW(jLoadFiles[i]->AsString().c_str()))
                    return false;
            }
        }

        // window
        if (joRoot.count(L"Window") > 0)
        {
            JSONObject joWindow = joRoot[L"Window"]->AsObject();
            this->mWindow.mFullScreen = joWindow[L"FullScreen"]->AsBool();
            this->mWindow.mPositionX = int(joWindow[L"PositionX"]->AsNumber());
            this->mWindow.mPositionY = int(joWindow[L"PositionY"]->AsNumber());
            this->mWindow.mWidth = int(joWindow[L"Width"]->AsNumber());
            this->mWindow.mHeight = int(joWindow[L"Height"]->AsNumber());
        }
        else
        {
            this->mWindow.mFullScreen = false;
            this->mWindow.mPositionX = 0;
            this->mWindow.mPositionY = 0;
            this->mWindow.mWidth = 1920;
            this->mWindow.mHeight = 1080;
        }

        // playback
        if (joRoot.count(L"Playback") > 0)
        {
            JSONObject joPlayback = joRoot[L"Playback"]->AsObject();
            {
                this->mPlayback.mLocation = iDeserializeTransform(joPlayback[L"Location"]);

                JSONObject joSpawn = joPlayback[L"PlayerSpawn"]->AsObject();
                if (joSpawn.find(L"Custom") != joSpawn.end())
                    this->mPlayback.mPlayerSpawn.mCustom = iDeserializeTransform(joSpawn[L"Custom"]);
                else
                    this->mPlayback.mPlayerSpawn.mCustom = ImmCore::trans3d::identity();

                this->mPlayback.mPlayerSpawn.mLocation.InitCopyW(joSpawn[L"Location"]->AsString().c_str());
            }
        }
        else
        {
            this->mPlayback.mLocation = ImmCore::trans3d::identity();
            this->mPlayback.mPlayerSpawn.mCustom = ImmCore::trans3d::identity();
            this->mPlayback.mPlayerSpawn.mLocation.InitCopyW(L"Spaces_Player1");
        }

        // Sound
        if (joRoot.count(L"Sound") > 0)
        {
            JSONObject joSnd = joRoot[L"Sound"]->AsObject();
            const std::wstring jDevice = joSnd[L"Device"]->AsString();
            if (!this->mSound.mDevice.InitCopyW(jDevice.c_str()))
                return false;
        }

        this->mUI.mEnableHaptics = true;
        this->mUI.mLeftHanded = false;
        this->mUI.UISoundVolume = 0.5f;
        if (joRoot.count(L"UI") > 0)
        {
            JSONObject joUI = joRoot[L"UI"]->AsObject();
            if (joUI.count(L"EnableHaptics") > 0)
                this->mUI.mEnableHaptics = joUI[L"EnableHaptics"]->AsBool();
            if (joUI.count(L"LeftHanded") > 0)
                this->mUI.mLeftHanded = joUI[L"LeftHanded"]->AsBool();
            if (joUI.count(L"UISoundVolume") > 0)
                this->mUI.UISoundVolume = float(joUI[L"UISoundVolume"]->AsNumber());
        }


        // rendering
        mRendering.mEnableVR = true;
        mRendering.mRenderingAPI = Rendering::API::GL;
        mRendering.mRenderingTechnique = Rendering::Technique::Static;
        mRendering.mPixelDensity = 1.0f;
        mRendering.mSupersampling = 1;
        if (joRoot.count(L"Rendering") > 0)
        {
            JSONObject jo = joRoot[L"Rendering"]->AsObject();
            mRendering.mPixelDensity = float(jo[L"PixelDensity"]->AsNumber());
            mRendering.mSupersampling = int(jo[L"Supersampling"]->AsNumber());

            mRendering.mEnableVR = jo[L"EnableVR"]->AsBool();

            if (jo.count(L"RenderingAPI") > 0)
            {
                std::wstring str = jo[L"RenderingAPI"]->AsString();
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                     if (str == L"opengl" ) mRendering.mRenderingAPI = Rendering::API::GL;
                else if (str == L"directx") mRendering.mRenderingAPI = Rendering::API::DX;
                else return false;
            }

            if (jo.count(L"RenderingTechnique") > 0)
            {
                std::wstring str = jo[L"RenderingTechnique"]->AsString();

                std::transform(str.begin(), str.end(), str.begin(), ::tolower);

                     if (str == L"static")         mRendering.mRenderingTechnique = Rendering::Technique::Static;
                else if (str == L"pretessellated") mRendering.mRenderingTechnique = Rendering::Technique::Pretessellated;
                else return false;
            }
        };

        free(json);
        return true;
    }
}
