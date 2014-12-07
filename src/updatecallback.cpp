#include "../include/updatecallback.hpp"

#include <iostream>
#include <string>

#include "Common/IntToString.h"
#include "Windows/PropVariant.h"
//#include "Windows/PropVariantConversions.h"

using namespace std;
using namespace Bit7z;

const std::wstring kEmptyFileAlias = L"[Content]";

UpdateCallback::UpdateCallback( const vector<FSItem>& dirItems ): mIsPasswordDefined( false ),
    mAskPassword( false ), mDirItems( dirItems )  {
    mNeedBeClosed = false;
    mFailedFiles.clear();
    mFailedCodes.Clear();
}
UpdateCallback::~UpdateCallback() { Finilize(); }

HRESULT UpdateCallback::SetTotal( UInt64 /* size */ ) {
    return S_OK;
}

HRESULT UpdateCallback::SetCompleted( const UInt64* /* completeValue */ ) {
    return S_OK;
}

HRESULT UpdateCallback::EnumProperties( IEnumSTATPROPSTG** /* enumerator */ ) {
    return E_NOTIMPL;
}

HRESULT UpdateCallback::GetUpdateItemInfo( UInt32 /* index */, Int32* newData,
                                           Int32* newProperties, UInt32* indexInArchive ) {
    if ( newData != NULL )
        *newData = BoolToInt( true );

    if ( newProperties != NULL )
        *newProperties = BoolToInt( true );

    if ( indexInArchive != NULL )
        *indexInArchive = ( UInt32 ) - 1;

    return S_OK;
}

HRESULT UpdateCallback::GetProperty( UInt32 index, PROPID propID, PROPVARIANT* value ) {
    NWindows::NCOM::CPropVariant prop;

    if ( propID == kpidIsAnti ) {
        prop = false;
        prop.Detach( value );
        return S_OK;
    }

    const FSItem dirItem = mDirItems[index];

    switch ( propID ) {
        case kpidPath  : prop = dirItem.relativePath().c_str(); break;
        case kpidIsDir : prop = dirItem.isDir(); break;
        case kpidSize  : prop = dirItem.size(); break;
        case kpidAttrib: prop = dirItem.attributes(); break;
        case kpidCTime : prop = dirItem.creationTime(); break;
        case kpidATime : prop = dirItem.lastAccessTime(); break;
        case kpidMTime : prop = dirItem.lastWriteTime(); break;
    }

    prop.Detach( value );
    return S_OK;
}

HRESULT UpdateCallback::Finilize() {
    if ( mNeedBeClosed ) {
        //cout << endl;
        mNeedBeClosed = false;
    }

    return S_OK;
}

static void GetStream2( const wstring& name ) {
    cout << "Compressing  '";

    if ( name.empty() )
        wcout << kEmptyFileAlias;
    else
        wcout << name;

    cout << "'" << endl;
}

HRESULT UpdateCallback::GetStream( UInt32 index, ISequentialInStream** inStream ) {
    RINOK( Finilize() );
    const FSItem dirItem = mDirItems[index];
    //GetStream2( dirItem.name() );

    if ( dirItem.isDir() )
        return S_OK;

    CInFileStream* inStreamSpec = new CInFileStream;
    CMyComPtr<ISequentialInStream> inStreamLoc( inStreamSpec );
    wstring path = dirItem.fullPath();
    //wcout << "path: " << path << endl;
    //wcout << "rpath: " << dirItem.relativePath() << endl;

    if ( !inStreamSpec->Open( path.c_str() ) ) {
        DWORD sysError = ::GetLastError();
        mFailedCodes.Add( sysError );
        mFailedFiles.push_back( path );
        // if (systemError == ERROR_SHARING_VIOLATION)
        {
            cerr << endl << "WARNING: can't open file (error " << sysError << ")" << endl;
            // PrintString(NError::MyFormatMessageW(systemError));
            return S_FALSE;
        }
        // return sysError;
    }

    *inStream = inStreamLoc.Detach();
    return S_OK;
}

HRESULT UpdateCallback::SetOperationResult( Int32 /* operationResult */ ) {
    mNeedBeClosed = true;
    return S_OK;
}

HRESULT UpdateCallback::GetVolumeSize( UInt32 index, UInt64* size ) {
    if ( mVolumesSizes.Size() == 0 )
        return S_FALSE;

    if ( index >= ( UInt32 )mVolumesSizes.Size() )
        index = mVolumesSizes.Size() - 1;

    *size = mVolumesSizes[index];
    return S_OK;
}

HRESULT UpdateCallback::GetVolumeStream( UInt32 index, ISequentialOutStream** volumeStream ) {
    wchar_t temp[16];
    ConvertUInt32ToString( index + 1, temp );
    wstring res = temp;

    while ( res.length() < 2 )
        res = L'0' + res;

    wstring fileName = mVolName;
    fileName += L'.';
    fileName += res;
    fileName += mVolExt;
    COutFileStream* streamSpec = new COutFileStream;
    CMyComPtr<ISequentialOutStream> streamLoc( streamSpec );

    if ( !streamSpec->Create( fileName.c_str(), false ) )
        return ::GetLastError();

    *volumeStream = streamLoc.Detach();
    return S_OK;
}

HRESULT UpdateCallback::CryptoGetTextPassword2( Int32* passwordIsDefined, BSTR* password ) {
    if ( !mIsPasswordDefined ) {
        if ( mAskPassword ) {
            // You can ask real password here from user
            // Password = GetPassword(OutStream);
            // PasswordIsDefined = true;
            cerr << "Password is not defined";
            return E_ABORT;
        }
    }

    *passwordIsDefined = BoolToInt( mIsPasswordDefined );
    return StringToBstr( mPassword.c_str(), password );
}
