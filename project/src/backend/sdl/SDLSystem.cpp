#include <system/System.h>

#ifdef HX_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef HX_WINDOWS
#include <shlobj.h>
#include <stdio.h>
//#include <io.h>
//#include <fcntl.h>
#ifdef __MINGW32__
#ifndef CSIDL_MYDOCUMENTS
#define CSIDL_MYDOCUMENTS CSIDL_PERSONAL
#endif
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif
#endif
#endif

#include <SDL_filesystem.h>
#include <SDL_rwops.h>
#include <SDL_timer.h>
#include <string>


namespace lime {
	
	
	const char* System::GetDirectory (SystemDirectory type) {
		
		switch (type) {
			
			case APPLICATION:
				
				// TODO: Need to get the user's company and title or package name
				// previous behavior was the company + file
				return SDL_GetPrefPath ("My Company", "My Awesome SDL 2 Game");
				break;
			
			case APPLICATION_STORAGE:
				
				return SDL_GetBasePath ();
				break;
			
			case DESKTOP: {
				
				#if defined (HX_WINRT)
				
				Windows::Storage::StorageFolder folder = Windows::Storage::KnownFolders::HomeGroup;
				std::wstring resultW (folder->Begin ());
				std::string result (resultW.begin (), resultW.end ());
				return result.c_str ();
				
				#elif defined (HX_WINDOWS)
				
				char result[MAX_PATH] = "";
				SHGetFolderPath (NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, result);
				return std::string (result).c_str ();
				
				#else
				
				std::string result = std::string (getenv ("HOME")) + std::string ("/Desktop");
				return result.c_str ();
				
				#endif
				break;
				
			}
			
			case DOCUMENTS: {
				
				#if defined (HX_WINRT)
				
				Windows::Storage::StorageFolder folder = Windows::Storage::KnownFolders::DocumentsLibrary;
				std::wstring resultW (folder->Begin ());
				std::string result (resultW.begin (), resultW.end ());
				return result.c_str ();
				
				#elif defined (HX_WINDOWS)
				
				char result[MAX_PATH] = "";
				SHGetFolderPath (NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, result);
				return std::string (result).c_str ();
				
				#else
				
				std::string result = std::string (getenv ("HOME")) + std::string ("/Documents");
				return result.c_str ();
				
				#endif
				break;
				
			}
			
			case USER: {
				
				#if defined (HX_WINRT)
				
				Windows::Storage::StorageFolder folder = Windows::Storage::ApplicationData::Current->RoamingFolder;
				std::wstring resultW (folder->Begin ());
				std::string result (resultW.begin (), resultW.end ());
				return result.c_str ();
				
				#elif defined (HX_WINDOWS)
				
				char result[MAX_PATH] = "";
				SHGetFolderPath (NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, result);
				return std::string (result).c_str ();
				
				#else
				
				std::string result = getenv ("HOME");
				return result.c_str ();
				
				#endif
				break;
				
			}
			
		}
		
		return 0;
		
	}
	
	
	double System::GetTimer () {
		
		return SDL_GetTicks ();
		
	}
	
	
	FILE* FILE_HANDLE::getFile () {
		
		#ifndef HX_WINDOWS
		
		switch (((SDL_RWops*)handle)->type) {
			
			case SDL_RWOPS_STDFILE:
				
				return ((SDL_RWops*)handle)->hidden.stdio.fp;
			
			case SDL_RWOPS_JNIFILE:
			{
				#ifdef ANDROID
				FILE* file = ::fdopen (((SDL_RWops*)handle)->hidden.androidio.fd, "rb");
				::fseek (file, ((SDL_RWops*)handle)->hidden.androidio.offset, 0);
				return file;
				#endif
			}
				
			case SDL_RWOPS_WINFILE:
			{
				/*#ifdef HX_WINDOWS
				printf("SDKFLJDSLFKJ\n");
				int fd = _open_osfhandle ((intptr_t)((SDL_RWops*)handle)->hidden.windowsio.h, _O_RDONLY);
				
				if (fd != -1) {
					printf("SDKFLJDSLFKJ\n");
					return ::fdopen (fd, "rb");
					
				}
				#endif*/
			}
			
		}
		
		return NULL;
		
		#else
		
		return (FILE*)handle;
		
		#endif
		
	}
	
	
	int FILE_HANDLE::getLength () {
		
		#ifndef HX_WINDOWS
		
		return SDL_RWsize (((SDL_RWops*)handle));
		
		#else
		
		return 0;
		
		#endif
		
	}
	
	
	bool FILE_HANDLE::isFile () {
		
		#ifndef HX_WINDOWS
		
		return ((SDL_RWops*)handle)->type == SDL_RWOPS_STDFILE;
		
		#else
		
		return true;
		
		#endif
		
	}
	
	
	int fclose (FILE_HANDLE *stream) {
		
		#ifndef HX_WINDOWS
		
		if (stream) {
			
			return SDL_RWclose ((SDL_RWops*)stream->handle);
			
		}
		
		return 0;
		
		#else
		
		return ::fclose ((FILE*)stream->handle);
		
		#endif
		
	}
	
	
	FILE_HANDLE *fdopen (int fd, const char *mode) {
		
		#ifndef HX_WINDOWS
		
		FILE* fp = ::fdopen (fd, mode);
		SDL_RWops *result = SDL_RWFromFP (fp, SDL_TRUE);
		
		if (result) {
			
			return new FILE_HANDLE (result);
			
		}
		
		return NULL;
		
		#else
		
		FILE* result = ::fdopen (fd, mode);
		
		if (result) {
			
			return new FILE_HANDLE (result);
			
		}
		
		return NULL;
		
		#endif
		
	}
	
	
	FILE_HANDLE *fopen (const char *filename, const char *mode) {
		
		#ifndef HX_WINDOWS
		
		SDL_RWops *result;
		
		#ifdef HX_MACOS
		
		result = SDL_RWFromFile (filename, "rb");
		
		if (!result) {
			
			CFStringRef str = CFStringCreateWithCString (NULL, filename, kCFStringEncodingUTF8);
			CFURLRef path = CFBundleCopyResourceURL (CFBundleGetMainBundle (), str, NULL, NULL);
			CFRelease (str);
			
			if (path) {
				
				str = CFURLCopyPath (path);
				CFIndex maxSize = CFStringGetMaximumSizeForEncoding (CFStringGetLength (str), kCFStringEncodingUTF8);
				char *buffer = (char *)malloc (maxSize);
				
				if (CFStringGetCString (str, buffer, maxSize, kCFStringEncodingUTF8)) {
					
					result = SDL_RWFromFP (::fopen (buffer, "rb"), SDL_TRUE);
					free (buffer);
					
				}
				
				CFRelease (str);
				CFRelease (path);
				
			}
			
		}
		#else
		result = SDL_RWFromFile (filename, mode);
		#endif
		
		if (result) {
			
			return new FILE_HANDLE (result);
			
		}
		
		return NULL;
		
		#else
		
		FILE* result = ::fopen (filename, mode);
		
		if (result) {
			
			return new FILE_HANDLE (result);
			
		}
		
		return NULL;
		
		#endif
		
	}
	
	
	size_t fread (void *ptr, size_t size, size_t count, FILE_HANDLE *stream) {
		
		#ifndef HX_WINDOWS
		
		return SDL_RWread (stream ? (SDL_RWops*)stream->handle : NULL, ptr, size, count);
		
		#else
		
		return ::fread (ptr, size, count, (FILE*)stream->handle);
		
		#endif
		
	}
	
	
	int fseek (FILE_HANDLE *stream, long int offset, int origin) {
		
		#ifndef HX_WINDOWS
		
		return SDL_RWseek (stream ? (SDL_RWops*)stream->handle : NULL, offset, origin);
		
		#else
		
		return ::fseek ((FILE*)stream->handle, offset, origin);
		
		#endif
		
	}
	
	
	long int ftell (FILE_HANDLE *stream) {
		
		#ifndef HX_WINDOWS
		
		return SDL_RWtell (stream ? (SDL_RWops*)stream->handle : NULL);
		
		#else
		
		return ::ftell ((FILE*)stream->handle);
		
		#endif
		
	}
	
	
	size_t fwrite (const void *ptr, size_t size, size_t count, FILE_HANDLE *stream) {
		
		#ifndef HX_WINDOWS
		
		return SDL_RWwrite (stream ? (SDL_RWops*)stream->handle : NULL, ptr, size, count);
		
		#else
		
		return ::fwrite (ptr, size, count, (FILE*)stream->handle);
		
		#endif
		
	}
	
	
}