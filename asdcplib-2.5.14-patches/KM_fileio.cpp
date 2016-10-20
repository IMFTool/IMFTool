/*
Copyright (c) 2004-2016, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
  /*! \file    KM_fileio.cpp
    \version $Id: KM_fileio.cpp,v 1.45 2016/05/06 18:40:33 jhurst Exp $
    \brief   portable file i/o
  */

#include <KM_fileio.h>
#include <KM_log.h>
#include <fcntl.h>

#include <assert.h>

#ifdef KM_WIN32
#include <direct.h>
#else
#define _getcwd getcwd
#define _unlink unlink
#define _rmdir rmdir
#endif

// only needed by GetExecutablePath()
#if defined(KM_MACOSX)
#include <mach-o/dyld.h>
#endif

using namespace Kumu;

#ifdef KM_WIN32
typedef struct _stati64 fstat_t;
#define S_IFLNK 0


// win32 has WriteFileGather() and ReadFileScatter() but they
// demand page alignment and page sizing, making them unsuitable
// for use with arbitrary buffer sizes.
struct iovec {
  char* iov_base; // stupid iovec uses char*
  int   iov_len;
};
#else
# if defined(__linux__)
#   include <sys/statfs.h>
# else
#  include <sys/param.h>
#  include <sys/mount.h>
# endif

#include <sys/stat.h>
#include <sys/uio.h>
typedef struct stat     fstat_t;
#endif

#if defined(__sun) && defined(__SVR4)
#include <sys/statfs.h>
#endif

//
static Kumu::Result_t
do_stat(const char* path, fstat_t* stat_info)
{
  KM_TEST_NULL_STR_L(path);
  KM_TEST_NULL_L(stat_info);

  Kumu::Result_t result = Kumu::RESULT_OK;

#ifdef KM_WIN32
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

  if ( _stati64(path, stat_info) == (__int64)-1 )
    result = Kumu::RESULT_FILEOPEN;

  ::SetErrorMode( prev );
#else
  if ( stat(path, stat_info) == -1L )
    result = Kumu::RESULT_FILEOPEN;

  if ( (stat_info->st_mode & (S_IFREG|S_IFLNK|S_IFDIR)) == 0 )
    result = Kumu::RESULT_FILEOPEN;
#endif

  return result;
}

#ifndef KM_WIN32

//
static Kumu::Result_t
do_fstat(FileHandle handle, fstat_t* stat_info)
{
  KM_TEST_NULL_L(stat_info);

  Kumu::Result_t result = Kumu::RESULT_OK;

  if ( fstat(handle, stat_info) == -1L )
    result = Kumu::RESULT_FILEOPEN;

  if ( (stat_info->st_mode & (S_IFREG|S_IFLNK|S_IFDIR)) == 0 )
    result = Kumu::RESULT_FILEOPEN;

  return result;
}

#endif


//
bool
Kumu::PathExists(const std::string& pathname)
{
  if ( pathname.empty() )
    return false;

  fstat_t info;

  if ( KM_SUCCESS(do_stat(pathname.c_str(), &info)) )
    return true;

  return false;
}

//
bool
Kumu::PathIsFile(const std::string& pathname)
{
  if ( pathname.empty() )
    return false;

  fstat_t info;

  if ( KM_SUCCESS(do_stat(pathname.c_str(), &info)) )
    {
      if ( info.st_mode & ( S_IFREG|S_IFLNK ) )
        return true;
    }

  return false;
}


//
bool
Kumu::PathIsDirectory(const std::string& pathname)
{
  if ( pathname.empty() )
    return false;

  fstat_t info;

  if ( KM_SUCCESS(do_stat(pathname.c_str(), &info)) )
    {
      if ( info.st_mode & S_IFDIR )
        return true;
    }

  return false;
}

//
Kumu::fsize_t
Kumu::FileSize(const std::string& pathname)
{
  if ( pathname.empty() )
    return 0;

  fstat_t info;

  if ( KM_SUCCESS(do_stat(pathname.c_str(), &info)) )
    {
      if ( info.st_mode & ( S_IFREG|S_IFLNK ) )
        return(info.st_size);
    }

  return 0;
}

//
static void
make_canonical_list(const PathCompList_t& in_list, PathCompList_t& out_list)
{
  PathCompList_t::const_iterator i;
  for ( i = in_list.begin(); i != in_list.end(); ++i )
    {
      if ( *i == ".." )
	{
	  if ( ! out_list.empty() )
	    {
	      out_list.pop_back();
	    }
	}
      else if ( *i != "." )
	{
	  out_list.push_back(*i);
	}
    }
}

//
std::string
Kumu::PathMakeCanonical(const std::string& Path, char separator)
{
  PathCompList_t in_list, out_list;
  bool is_absolute = PathIsAbsolute(Path, separator);
  PathToComponents(Path, in_list, separator);
  make_canonical_list(in_list, out_list);

  if ( is_absolute )
    return ComponentsToAbsolutePath(out_list, separator);

  return ComponentsToPath(out_list, separator);
}

//
bool
Kumu::PathsAreEquivalent(const std::string& lhs, const std::string& rhs)
{
  return PathMakeAbsolute(lhs) == PathMakeAbsolute(rhs);
}

//
Kumu::PathCompList_t&
Kumu::PathToComponents(const std::string& path, PathCompList_t& component_list, char separator)
{
  std::string s;
  s = separator;
  PathCompList_t tmp_list = km_token_split(path, std::string(s));
  PathCompList_t::const_iterator i;

  for ( i = tmp_list.begin(); i != tmp_list.end(); ++i )
    {
      if ( ! i->empty() )
	{
	  component_list.push_back(*i);
	}
    }

  return component_list;
}

//
std::string
Kumu::ComponentsToPath(const PathCompList_t& CList, char separator)
{
  if ( CList.empty() )
    return "";

  PathCompList_t::const_iterator ci = CList.begin();
  std::string out_path = *ci;

  for ( ci++; ci != CList.end(); ci++ )
    out_path += separator + *ci;

  return out_path;
}

//
std::string
Kumu::ComponentsToAbsolutePath(const PathCompList_t& CList, char separator)
{
  std::string out_path;

  if ( CList.empty() )
    out_path = separator;
  else
    {
      PathCompList_t::const_iterator ci;

      for ( ci = CList.begin(); ci != CList.end(); ci++ )
	out_path += separator + *ci;
    }

  return out_path;
}

//
bool
Kumu::PathHasComponents(const std::string& Path, char separator)
{
  if ( strchr(Path.c_str(), separator) == 0 )
    return false;

  return true;
}

//
bool
Kumu::PathIsAbsolute(const std::string& Path, char separator)
{
  if ( Path.empty() )
    return false;

  if ( Path[0] == separator)
    return true;

  return false;
}

//
std::string
Kumu::PathCwd()
{
  char cwd_buf [MaxFilePath];
  if ( _getcwd(cwd_buf, MaxFilePath) == 0 )
    {
      DefaultLogSink().Error("Error retrieving current working directory.");
      return "";
    }

  return cwd_buf;
}

//
std::string
Kumu::PathMakeAbsolute(const std::string& Path, char separator)
{
  if ( Path.empty() )
    {
      std::string tmpstr;
      tmpstr = separator;
      return tmpstr;
    }

  if ( PathIsAbsolute(Path, separator) )
    return PathMakeCanonical(Path);

  PathCompList_t in_list, out_list;
  PathToComponents(PathJoin(PathCwd(), Path), in_list);
  make_canonical_list(in_list, out_list);

  return ComponentsToAbsolutePath(out_list);
}

//
std::string
Kumu::PathMakeLocal(const std::string& Path, const std::string& Parent)
{
  size_t pos = Path.find(Parent);

  if ( pos == 0 ) // Parent found at offset 0
    return Path.substr(Parent.size()+1);

  return Path;
}

//
std::string
Kumu::PathBasename(const std::string& Path, char separator)
{
  PathCompList_t CList;
  PathToComponents(Path, CList, separator);

  if ( CList.empty() )
    return "";

  return CList.back();
}

//
std::string
Kumu::PathDirname(const std::string& Path, char separator)
{
  PathCompList_t CList;
  bool is_absolute = PathIsAbsolute(Path, separator);
  PathToComponents(Path, CList, separator);

  if ( CList.empty() )
    return is_absolute ? "/" : "";

  CList.pop_back();

  if ( is_absolute )
    return ComponentsToAbsolutePath(CList, separator);

  return ComponentsToPath(CList, separator);
}

//
std::string
Kumu::PathGetExtension(const std::string& Path)
{
  std::string Basename = PathBasename(Path);
  const char* p = strrchr(Basename.c_str(), '.'); 

  if ( p++ == 0 )
    return "";

  return p;
}

//
std::string
Kumu::PathSetExtension(const std::string& Path, const std::string& Extension) // empty extension removes
{
  std::string Basename = PathBasename(Path);
  const char* p = strrchr(Basename.c_str(), '.'); 

  if ( p != 0 )
    Basename = Basename.substr(0, p - Basename.c_str());

  if ( Extension.empty() )
    return Basename;

  return Basename + "." + Extension;
}

//
std::string
Kumu::PathJoin(const std::string& Path1, const std::string& Path2, char separator)
{
  return Path1 + separator + Path2;
}

//
std::string
Kumu::PathJoin(const std::string& Path1, const std::string& Path2, const std::string& Path3, char separator)
{
  return Path1 + separator + Path2 + separator + Path3;
}

//
std::string
Kumu::PathJoin(const std::string& Path1, const std::string& Path2,
	       const std::string& Path3, const std::string& Path4, char separator)
{
  return Path1 + separator + Path2 + separator + Path3 + separator + Path4;
}

#ifndef KM_WIN32
// returns false if link cannot be read
//
bool
Kumu::PathResolveLinks(const std::string& link_path, std::string& resolved_path, char separator)
{
  PathCompList_t in_list, out_list;
  PathToComponents(PathMakeCanonical(link_path), in_list, separator);
  PathCompList_t::iterator i;
  char link_buf[MaxFilePath];

  for ( i = in_list.begin(); i != in_list.end(); ++i )
    {
      assert ( *i != ".." && *i != "." );
      out_list.push_back(*i);

      for (;;)
	{
	  std::string next_link = ComponentsToAbsolutePath(out_list, separator);
	  ssize_t link_size = readlink(next_link.c_str(), link_buf, MaxFilePath);

	  if ( link_size == -1 )
	    {
	      if ( errno == EINVAL )
		break;

	      DefaultLogSink().Error("%s: readlink: %s\n", next_link.c_str(), strerror(errno));
	      return false;
	    }
	  
	  assert(link_size < MaxFilePath);
	  link_buf[link_size] = 0;
	  std::string tmp_path;
	  out_list.clear();

	  if ( PathIsAbsolute(link_buf) )
	    {
	      tmp_path = link_buf;
	    }
	  else
	    {
	      tmp_path = PathJoin(PathDirname(next_link), link_buf);
	    }

	  PathToComponents(PathMakeCanonical(tmp_path), out_list, separator);
	}
    }

  resolved_path = ComponentsToAbsolutePath(out_list, separator);
  return true;
}

#else // KM_WIN32
// TODO: is there a reasonable equivalent to be written for win32?
//
bool
Kumu::PathResolveLinks(const std::string& link_path, std::string& resolved_path, char separator)
{
  resolved_path = link_path;
  return true;
}
#endif

//
Kumu::PathList_t&
Kumu::FindInPaths(const IPathMatch& Pattern, const Kumu::PathList_t& SearchPaths,
		  Kumu::PathList_t& FoundPaths, bool one_shot, char separator)
{
  PathList_t::const_iterator si;
  for ( si = SearchPaths.begin(); si != SearchPaths.end(); si++ )
    {
      FindInPath(Pattern, *si, FoundPaths, one_shot, separator);

      if ( one_shot && ! FoundPaths.empty() )
	break;
    }

  return FoundPaths;
}

//
Kumu::PathList_t&
Kumu::FindInPath(const IPathMatch& Pattern, const std::string& SearchDir,
		  Kumu::PathList_t& FoundPaths, bool one_shot, char separator)
{
  char name_buf[MaxFilePath];
  DirScanner Dir;

  if ( KM_SUCCESS(Dir.Open(SearchDir.c_str())) )
    {
      while ( KM_SUCCESS(Dir.GetNext(name_buf)) )
	{
	  if ( name_buf[0] == '.' ) continue; // no hidden files
	  std::string tmp_path = SearchDir + separator + name_buf;

	  if ( PathIsDirectory(tmp_path.c_str()) )
	    FindInPath(Pattern, tmp_path, FoundPaths, one_shot, separator);
	  
	  else if ( Pattern.Match(name_buf) )
	    {
	      FoundPaths.push_back(SearchDir + separator + name_buf);
	      if ( one_shot )
		break;
	    }
	}
    }

  return FoundPaths;
}


#ifndef KM_WIN32

//
Kumu::PathMatchRegex::PathMatchRegex(const std::string& s)
{
  int result = regcomp(&m_regex, s.c_str(), REG_NOSUB); // (REG_EXTENDED|REG_NOSUB|REG_NEWLINE));

  if ( result )
    {
      char buf[128];
      regerror(result, &m_regex, buf, 128);
      DefaultLogSink().Error("PathMatchRegex: %s\n", buf);
      regfree(&m_regex);
    }
}

Kumu::PathMatchRegex::PathMatchRegex(const PathMatchRegex& rhs) : IPathMatch() {
  m_regex = rhs.m_regex;
}

Kumu::PathMatchRegex::~PathMatchRegex() {
  regfree(&m_regex);
}

bool
Kumu::PathMatchRegex::Match(const std::string& s) const {
  return ( regexec(&m_regex, s.c_str(), 0, 0, 0) == 0 );
}



//
Kumu::PathMatchGlob::PathMatchGlob(const std::string& glob)
{
  std::string regex; // convert glob to regex

  for ( const char* p = glob.c_str(); *p != 0; p++ )
    {
      switch (*p)
	{
	case '.':  regex += "\\.";  break;
	case '*':  regex += ".*";   break;
	case '?':  regex += ".?";   break;
	default:   regex += *p;
	}
    }
  regex += '$';

  int result = regcomp(&m_regex, regex.c_str(), REG_NOSUB);

  if ( result )
    {
      char buf[128];
      regerror(result, &m_regex, buf, 128);
      DefaultLogSink().Error("PathMatchRegex: %s\n", buf);
      regfree(&m_regex);
    }
}

Kumu::PathMatchGlob::PathMatchGlob(const PathMatchGlob& rhs) : IPathMatch() {
  m_regex = rhs.m_regex;
}

Kumu::PathMatchGlob::~PathMatchGlob() {
  regfree(&m_regex);
}

bool
Kumu::PathMatchGlob::Match(const std::string& s) const {
  return ( regexec(&m_regex, s.c_str(), 0, 0, 0) == 0 );
}

#endif


//------------------------------------------------------------------------------------------

#define X_BUFSIZE 1024

//
std::string
Kumu::GetExecutablePath(const std::string& default_path)
{
  char path[X_BUFSIZE] = {0};
  bool success = false;

#if defined(KM_WIN32)
  DWORD size = X_BUFSIZE;
  DWORD rc = GetModuleFileName(0, path, size);
  success = ( rc != 0 );
#elif defined(KM_MACOSX)
  uint32_t size = X_BUFSIZE;
  int rc = _NSGetExecutablePath(path, &size);
  success = ( rc != -1 );
#elif defined(__linux__)
  size_t size = X_BUFSIZE;
  ssize_t rc = readlink("/proc/self/exe", path, size);
  success = ( rc != -1 );
#elif defined(__OpenBSD__) || defined(__FreeBSD__)
  size_t size = X_BUFSIZE;
  ssize_t rc = readlink("/proc/curproc/file", path, size);
  success = ( rc != -1 );
#elif defined(__FreeBSD__)
  size_t size = X_BUFSIZE;
  ssize_t rc = readlink("/proc/curproc/file", path, size);
  success = ( rc != -1 );
#elif defined(__NetBSD__)
  size_t size = X_BUFSIZE;
  ssize_t rc = readlink("/proc/curproc/file", path, size);
  success = ( rc != -1 );
#elif defined(__sun) && defined(__SVR4)
  size_t size = X_BUFSIZE;
  char program[MAXPATHLEN];
  snprintf(program, MAXPATHLEN, "/proc/%d/path/a.out", getpid());
  ssize_t rc = readlink(program, path, size);
#else
#error GetExecutablePath --> Create a method for obtaining the executable name
#endif

  if ( success )
    {
      return Kumu::PathMakeCanonical(path);
    }

  return default_path;
}


//------------------------------------------------------------------------------------------
// portable aspects of the file classes

const int IOVecMaxEntries = 32; // we never use more that 3, but that number seems somehow small...

//
class Kumu::FileWriter::h__iovec
{
public:
  int            m_Count;
  struct iovec   m_iovec[IOVecMaxEntries];
  h__iovec() : m_Count(0) {}
};



//
Kumu::fsize_t
Kumu::FileReader::Size() const
{
#ifdef KM_WIN32
  return FileSize(m_Filename.c_str());
#else
  fstat_t info;

  if ( KM_SUCCESS(do_fstat(m_Handle, &info)) )
    {
      if ( info.st_mode & ( S_IFREG|S_IFLNK ) )
        return(info.st_size);
    }
#endif

  return 0;
}

// these are declared here instead of in the header file
// because we have a mem_ptr that is managing a hidden class
Kumu::FileWriter::FileWriter() {}
Kumu::FileWriter::~FileWriter() {}

//
Kumu::Result_t
Kumu::FileWriter::Writev(const byte_t* buf, ui32_t buf_len)
{
  assert( ! m_IOVec.empty() );
  register h__iovec* iov = m_IOVec;
  KM_TEST_NULL_L(buf);

  if ( iov->m_Count >= IOVecMaxEntries )
    {
      DefaultLogSink().Error("The iovec is full! Only %u entries allowed before a flush.\n",
			     IOVecMaxEntries);
      return RESULT_WRITEFAIL;
    }

  iov->m_iovec[iov->m_Count].iov_base = (char*)buf; // stupid iovec uses char*
  iov->m_iovec[iov->m_Count].iov_len = buf_len;
  iov->m_Count++;

  return RESULT_OK;
}


#ifdef KM_WIN32
//------------------------------------------------------------------------------------------
//

Kumu::Result_t
Kumu::FileReader::OpenRead(const std::string& filename) const
{
  const_cast<FileReader*>(this)->m_Filename = filename;
  
  // suppress popup window on error
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

  const_cast<FileReader*>(this)->m_Handle = ::CreateFileA(filename.c_str(),
			  (GENERIC_READ),                // open for reading
			  FILE_SHARE_READ,               // share for reading
			  NULL,                          // no security
			  OPEN_EXISTING,                 // read
			  FILE_ATTRIBUTE_NORMAL,         // normal file
			  NULL                           // no template file
			  );

  ::SetErrorMode(prev);

  return ( m_Handle == INVALID_HANDLE_VALUE ) ?
    Kumu::RESULT_FILEOPEN : Kumu::RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileReader::Close() const
{
  if ( m_Handle == INVALID_HANDLE_VALUE )
    return Kumu::RESULT_FILEOPEN;

  // suppress popup window on error
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  BOOL result = ::CloseHandle(m_Handle);
  ::SetErrorMode(prev);
  const_cast<FileReader*>(this)->m_Handle = INVALID_HANDLE_VALUE;

  return ( result == 0 ) ? Kumu::RESULT_FAIL : Kumu::RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileReader::Seek(Kumu::fpos_t position, SeekPos_t whence) const
{
  if ( m_Handle == INVALID_HANDLE_VALUE )
    return Kumu::RESULT_STATE;

  LARGE_INTEGER in;
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  in.QuadPart = position;
  in.LowPart = ::SetFilePointer(m_Handle, in.LowPart, &in.HighPart, whence);
  HRESULT LastError = GetLastError();
  ::SetErrorMode(prev);

  if ( (LastError != NO_ERROR
	&& (in.LowPart == INVALID_SET_FILE_POINTER
	    || in.LowPart == ERROR_NEGATIVE_SEEK )) )
    return Kumu::RESULT_READFAIL;
  
  return Kumu::RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileReader::Tell(Kumu::fpos_t* pos) const
{
  KM_TEST_NULL_L(pos);

  if ( m_Handle == INVALID_HANDLE_VALUE )
    return Kumu::RESULT_FILEOPEN;

  LARGE_INTEGER in;
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  in.QuadPart = (__int64)0;
  in.LowPart = ::SetFilePointer(m_Handle, in.LowPart, &in.HighPart, FILE_CURRENT);
  HRESULT LastError = GetLastError();
  ::SetErrorMode(prev);

  if ( (LastError != NO_ERROR
	&& (in.LowPart == INVALID_SET_FILE_POINTER
	    || in.LowPart == ERROR_NEGATIVE_SEEK )) )
    return Kumu::RESULT_READFAIL;

  *pos = (Kumu::fpos_t)in.QuadPart;
  return Kumu::RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileReader::Read(byte_t* buf, ui32_t buf_len, ui32_t* read_count) const
{
  KM_TEST_NULL_L(buf);
  Result_t result = Kumu::RESULT_OK;
  DWORD    tmp_count;
  ui32_t tmp_int;

  if ( read_count == 0 )
    read_count = &tmp_int;

  *read_count = 0;

  if ( m_Handle == INVALID_HANDLE_VALUE )
    return Kumu::RESULT_FILEOPEN;
  
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  if ( ::ReadFile(m_Handle, buf, buf_len, &tmp_count, NULL) == 0 )
    result = Kumu::RESULT_READFAIL;

  ::SetErrorMode(prev);

  if ( tmp_count == 0 ) /* EOF */
    result = Kumu::RESULT_ENDOFFILE;

  if ( KM_SUCCESS(result) )
    *read_count = tmp_count;

  return result;
}



//------------------------------------------------------------------------------------------
//

//
Kumu::Result_t
Kumu::FileWriter::OpenWrite(const std::string& filename)
{
  m_Filename = filename;
  
  // suppress popup window on error
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

  m_Handle = ::CreateFileA(filename.c_str(),
			  (GENERIC_WRITE|GENERIC_READ),  // open for reading
			  FILE_SHARE_READ,               // share for reading
			  NULL,                          // no security
			  CREATE_ALWAYS,                 // overwrite (beware!)
			  FILE_ATTRIBUTE_NORMAL,         // normal file
			  NULL                           // no template file
			  );

  ::SetErrorMode(prev);

  if ( m_Handle == INVALID_HANDLE_VALUE )
    return Kumu::RESULT_FILEOPEN;
  
  m_IOVec = new h__iovec;
  return Kumu::RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileWriter::Writev(ui32_t* bytes_written)
{
  assert( ! m_IOVec.empty() );
  register h__iovec* iov = m_IOVec;
  ui32_t tmp_int;

  if ( bytes_written == 0 )
    bytes_written = &tmp_int;

  if ( m_Handle == INVALID_HANDLE_VALUE )
    return Kumu::RESULT_STATE;

  *bytes_written = 0;
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  Result_t result = Kumu::RESULT_OK;

  // AFAIK, there is no writev() equivalent in the win32 API
  for ( register int i = 0; i < iov->m_Count; i++ )
    {
      ui32_t tmp_count = 0;
      BOOL wr_result = ::WriteFile(m_Handle,
				   iov->m_iovec[i].iov_base,
				   iov->m_iovec[i].iov_len,
				   (DWORD*)&tmp_count,
				   NULL);

      if ( wr_result == 0 || tmp_count != iov->m_iovec[i].iov_len)
	{
	  result = Kumu::RESULT_WRITEFAIL;
	  break;
	}

      *bytes_written += tmp_count;
    }

  ::SetErrorMode(prev);
  iov->m_Count = 0; // error nor not, all is lost

  return result;
}

//
Kumu::Result_t
Kumu::FileWriter::Write(const byte_t* buf, ui32_t buf_len, ui32_t* bytes_written)
{
  KM_TEST_NULL_L(buf);
  ui32_t tmp_int;

  if ( bytes_written == 0 )
    bytes_written = &tmp_int;

  if ( m_Handle == INVALID_HANDLE_VALUE )
    return Kumu::RESULT_STATE;

  // suppress popup window on error
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  BOOL result = ::WriteFile(m_Handle, buf, buf_len, (DWORD*)bytes_written, NULL);
  ::SetErrorMode(prev);

  if ( result == 0 || *bytes_written != buf_len )
    return Kumu::RESULT_WRITEFAIL;

  return Kumu::RESULT_OK;
}

#else // KM_WIN32
//------------------------------------------------------------------------------------------
// POSIX

//
Kumu::Result_t
Kumu::FileReader::OpenRead(const std::string& filename) const
{
  const_cast<FileReader*>(this)->m_Filename = filename;
  const_cast<FileReader*>(this)->m_Handle = open(filename.c_str(), O_RDONLY, 0);
  return ( m_Handle == -1L ) ? RESULT_FILEOPEN : RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileReader::Close() const
{
  if ( m_Handle == -1L )
    return RESULT_FILEOPEN;

  close(m_Handle);
  const_cast<FileReader*>(this)->m_Handle = -1L;
  return RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileReader::Seek(Kumu::fpos_t position, SeekPos_t whence) const
{
  if ( m_Handle == -1L )
    return RESULT_FILEOPEN;

  if ( lseek(m_Handle, position, whence) == -1L )
    return RESULT_BADSEEK;

  return RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileReader::Tell(Kumu::fpos_t* pos) const
{
  KM_TEST_NULL_L(pos);

  if ( m_Handle == -1L )
    return RESULT_FILEOPEN;

  Kumu::fpos_t tmp_pos;

  if (  (tmp_pos = lseek(m_Handle, 0, SEEK_CUR)) == -1 )
    return RESULT_READFAIL;

  *pos = tmp_pos;
  return RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileReader::Read(byte_t* buf, ui32_t buf_len, ui32_t* read_count) const
{
  KM_TEST_NULL_L(buf);
  i32_t  tmp_count = 0;
  ui32_t tmp_int = 0;

  if ( read_count == 0 )
    read_count = &tmp_int;

  *read_count = 0;

  if ( m_Handle == -1L )
    return RESULT_FILEOPEN;

  if ( (tmp_count = read(m_Handle, buf, buf_len)) == -1L )
    return RESULT_READFAIL;

  *read_count = tmp_count;
  return (tmp_count == 0 ? RESULT_ENDOFFILE : RESULT_OK);
}


//------------------------------------------------------------------------------------------
//

//
Kumu::Result_t
Kumu::FileWriter::OpenWrite(const std::string& filename)
{
  m_Filename = filename;
  m_Handle = open(filename.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0666);

  if ( m_Handle == -1L )
    {
      DefaultLogSink().Error("Error opening file %s: %s\n", filename.c_str(), strerror(errno));
      return RESULT_FILEOPEN;
    }

  m_IOVec = new h__iovec;
  return RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileWriter::OpenModify(const std::string& filename)
{
  m_Filename = filename;
  m_Handle = open(filename.c_str(), O_RDWR|O_CREAT, 0666);

  if ( m_Handle == -1L )
    {
      DefaultLogSink().Error("Error opening file %s: %s\n", filename.c_str(), strerror(errno));
      return RESULT_FILEOPEN;
    }

  m_IOVec = new h__iovec;
  return RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileWriter::Writev(ui32_t* bytes_written)
{
  assert( ! m_IOVec.empty() );
  register h__iovec* iov = m_IOVec;
  ui32_t tmp_int;

  if ( bytes_written == 0 )
    bytes_written = &tmp_int;

  if ( m_Handle == -1L )
    return RESULT_STATE;

  int total_size = 0;
  for ( int i = 0; i < iov->m_Count; i++ )
    total_size += iov->m_iovec[i].iov_len;

  int write_size = writev(m_Handle, iov->m_iovec, iov->m_Count);
  
  if ( write_size == -1L || write_size != total_size )
    return RESULT_WRITEFAIL;

  iov->m_Count = 0;
  *bytes_written = write_size;  
  return RESULT_OK;
}

//
Kumu::Result_t
Kumu::FileWriter::Write(const byte_t* buf, ui32_t buf_len, ui32_t* bytes_written)
{
  KM_TEST_NULL_L(buf);
  ui32_t tmp_int;

  if ( bytes_written == 0 )
    bytes_written = &tmp_int;

  if ( m_Handle == -1L )
    return RESULT_STATE;

  int write_size = write(m_Handle, buf, buf_len);

  if ( write_size == -1L || (ui32_t)write_size != buf_len )
    return RESULT_WRITEFAIL;

  *bytes_written = write_size;
  return RESULT_OK;
}


#endif // KM_WIN32

//------------------------------------------------------------------------------------------


//
Kumu::Result_t
Kumu::ReadFileIntoString(const std::string& filename, std::string& outString, ui32_t max_size)
{
  fsize_t    fsize = 0;
  ui32_t     read_size = 0;
  FileReader File;
  ByteString ReadBuf;

  Result_t result = File.OpenRead(filename);

  if ( KM_SUCCESS(result) )
    {
      fsize = File.Size();

      if ( fsize > (Kumu::fpos_t)max_size )
	{
	  DefaultLogSink().Error("%s: exceeds available buffer size (%u)\n", filename.c_str(), max_size);
	  return RESULT_ALLOC;
	}

      if ( fsize == 0 )
	{
	  DefaultLogSink().Error("%s: zero file size\n", filename.c_str());
	  return RESULT_READFAIL;
	}

      result = ReadBuf.Capacity((ui32_t)fsize);
    }

  if ( KM_SUCCESS(result) )
    result = File.Read(ReadBuf.Data(), ReadBuf.Capacity(), &read_size);

  if ( KM_SUCCESS(result) )
    outString.assign((const char*)ReadBuf.RoData(), read_size);

  return result;
}


//
Kumu::Result_t
Kumu::WriteStringIntoFile(const std::string& filename, const std::string& inString)
{
  FileWriter File;
  ui32_t write_count = 0;

  Result_t result = File.OpenWrite(filename);

  if ( KM_SUCCESS(result) )
    result = File.Write((byte_t*)inString.c_str(), inString.length(), &write_count);

  return result;
}

//------------------------------------------------------------------------------------------


//
Kumu::Result_t
Kumu::ReadFileIntoObject(const std::string& Filename, Kumu::IArchive& Object, ui32_t max_size)
{
  ByteString Buffer;
  ui32_t file_size = static_cast<ui32_t>(FileSize(Filename));
  Result_t result = Buffer.Capacity(file_size);

  if ( KM_SUCCESS(result) )
    {
      ui32_t read_count = 0;
      FileReader Reader;

      result = Reader.OpenRead(Filename);

      if ( KM_SUCCESS(result) )
	result = Reader.Read(Buffer.Data(), file_size, &read_count);
    
      if ( KM_SUCCESS(result) )
	{
	  assert(file_size == read_count);
	  Buffer.Length(read_count);
	  MemIOReader MemReader(&Buffer);
	  result = Object.Unarchive(&MemReader) ? RESULT_OK : RESULT_READFAIL;
	}
    }

  return result;
}

//
Kumu::Result_t
Kumu::WriteObjectIntoFile(const Kumu::IArchive& Object, const std::string& Filename)
{
  ByteString Buffer;
  Result_t result = Buffer.Capacity(Object.ArchiveLength());

  if ( KM_SUCCESS(result) )
    {
      ui32_t write_count = 0;
      FileWriter Writer;
      MemIOWriter MemWriter(&Buffer);

      result = Object.Archive(&MemWriter) ? RESULT_OK : RESULT_WRITEFAIL;

      if ( KM_SUCCESS(result) )
	{
	  Buffer.Length(MemWriter.Length());
	  result = Writer.OpenWrite(Filename);
	}

      if ( KM_SUCCESS(result) )
	result = Writer.Write(Buffer.RoData(), Buffer.Length(), &write_count);
    }

  return result;
}

//------------------------------------------------------------------------------------------
//

//
Result_t
Kumu::ReadFileIntoBuffer(const std::string& Filename, Kumu::ByteString& Buffer, ui32_t max_size)
{
  ui32_t file_size = FileSize(Filename);
  Result_t result = Buffer.Capacity(file_size);

  if ( KM_SUCCESS(result) )
    {
      ui32_t read_count = 0;
      FileReader Reader;

      result = Reader.OpenRead(Filename);

      if ( KM_SUCCESS(result) )
	result = Reader.Read(Buffer.Data(), file_size, &read_count);
    
      if ( KM_SUCCESS(result) )
	{
	  if ( file_size != read_count) 
	    return RESULT_READFAIL;

	  Buffer.Length(read_count);
	}
    }
  
  return result;
}

//
Result_t
Kumu::WriteBufferIntoFile(const Kumu::ByteString& Buffer, const std::string& Filename)
{
  ui32_t write_count = 0;
  FileWriter Writer;

  Result_t result = Writer.OpenWrite(Filename);

  if ( KM_SUCCESS(result) )
    result = Writer.Write(Buffer.RoData(), Buffer.Length(), &write_count);

  if ( KM_SUCCESS(result) && Buffer.Length() != write_count) 
    return RESULT_WRITEFAIL;

  return result;
}

//------------------------------------------------------------------------------------------
//


// Win32 directory scanner
//
#ifdef UNDEF // KM_WIN32

//
Kumu::DirScanner::DirScanner(void) : m_Handle(-1) {}

//
//
Result_t
Kumu::DirScanner::Open(const std::string& filename)
{
  // we need to append a '*' to read the entire directory
  ui32_t fn_len = filename.size(); 
  char* tmp_file = (char*)malloc(fn_len + 8);

  if ( tmp_file == 0 )
    return RESULT_ALLOC;

  strcpy(tmp_file, filename.c_str());
  char* p = &tmp_file[fn_len] - 1;

  if ( *p != '/' && *p != '\\' )
    {
      p++;
      *p++ = '/';
    }

  *p++ = '*';
  *p = 0;
  // whew...

  m_Handle = _findfirsti64(tmp_file, &m_FileInfo);
  Result_t result = RESULT_OK;

  if ( m_Handle == -1 )
    result = RESULT_NOT_FOUND;

  return result;
}


//
//
Result_t
Kumu::DirScanner::Close()
{
  if ( m_Handle == -1 )
    return RESULT_FILEOPEN;

  if ( _findclose((long)m_Handle) == -1 )
    return RESULT_FAIL;

  m_Handle = -1;
  return RESULT_OK;
}


// This sets filename param to the same per-instance buffer every time, so
// the value will change on the next call
Result_t
Kumu::DirScanner::GetNext(char* filename)
{
  KM_TEST_NULL_L(filename);

  if ( m_Handle == -1 )
    return RESULT_FILEOPEN;

  if ( m_FileInfo.name[0] == '\0' )
    return RESULT_ENDOFFILE;

  strncpy(filename, m_FileInfo.name, MaxFilePath);
  Result_t result = RESULT_OK;

  if ( _findnexti64((long)m_Handle, &m_FileInfo) == -1 )
    {
      m_FileInfo.name[0] = '\0';
	  
      if ( errno != ENOENT )
	result = RESULT_FAIL;
    }

  return result;
}


//
Kumu::DirScannerEx::DirScannerEx() : m_Handle(0) {}

//
Result_t
Kumu::DirScannerEx::Open(const std::string& dirname)
{
  Kumu::DefaultLogSink().Critical("Kumu::DirScannerEx unimplemented for Win32 API.\n");
  return RESULT_NOTIMPL;
}

//
Result_t
Kumu::DirScannerEx::Close()
{
  Kumu::DefaultLogSink().Critical("Kumu::DirScannerEx unimplemented for Win32 API.\n");
  return RESULT_NOTIMPL;
}

//
Result_t
Kumu::DirScannerEx::GetNext(std::string& next_item_name, DirectoryEntryType_t& next_item_type)
{
  Kumu::DefaultLogSink().Critical("Kumu::DirScannerEx unimplemented for Win32 API.\n");
  return RESULT_NOTIMPL;
}

#else // UNDEF //was:  KM_WIN32

// POSIX directory scanner

//
Kumu::DirScanner::DirScanner(void) : m_Handle(NULL) {}

//
Result_t
Kumu::DirScanner::Open(const std::string& dirname)
{
  Result_t result = RESULT_OK;

  if ( ( m_Handle = opendir(dirname.c_str()) ) == NULL )
    {
      switch ( errno )
	{
	case ENOENT:
	case ENOTDIR:
	  result = RESULT_NOTAFILE;
	case EACCES:
	  result = RESULT_NO_PERM;
	case ELOOP:
	case ENAMETOOLONG:
	  result = RESULT_PARAM;
	case EMFILE:
	case ENFILE:
	  result = RESULT_STATE;
	default:
	  DefaultLogSink().Error("DirScanner::Open(%s): %s\n", dirname.c_str(), strerror(errno));
	  result = RESULT_FAIL;
	}
    }

  return result;
}


//
Result_t
Kumu::DirScanner::Close()
{
  if ( m_Handle == NULL )
    return RESULT_FILEOPEN;

  if ( closedir(m_Handle) == -1 ) {
    switch ( errno )
      {
      case EBADF:
      case EINTR:
	return RESULT_STATE;
      default:
	DefaultLogSink().Error("DirScanner::Close(): %s\n", strerror(errno));
	return RESULT_FAIL;
      }
  }

  m_Handle = NULL;
  return RESULT_OK;
}


//
Result_t
Kumu::DirScanner::GetNext(char* filename)
{
  KM_TEST_NULL_L(filename);

  if ( m_Handle == NULL )
    return RESULT_FILEOPEN;

  struct dirent* entry;

  for (;;)
    {
      if ( ( entry = readdir(m_Handle)) == NULL )
	return RESULT_ENDOFFILE;

      break;
    }

  strncpy(filename, entry->d_name, MaxFilePath);
  return RESULT_OK;
}

//------------------------------------------------------------------------------------------

//
Kumu::DirScannerEx::DirScannerEx() : m_Handle(0) {}

//
Result_t
Kumu::DirScannerEx::Open(const std::string& dirname)
{
  Result_t result = RESULT_OK;

  if ( ( m_Handle = opendir(dirname.c_str()) ) == 0 )
    {
      switch ( errno )
	{
	case ENOENT:
	case ENOTDIR:
	  result = RESULT_NOTAFILE;
	case EACCES:
	  result = RESULT_NO_PERM;
	case ELOOP:
	case ENAMETOOLONG:
	  result = RESULT_PARAM;
	case EMFILE:
	case ENFILE:
	  result = RESULT_STATE;
	default:
	  DefaultLogSink().Error("DirScanner::Open(%s): %s\n", dirname.c_str(), strerror(errno));
	  result = RESULT_FAIL;
	}
    }

  if ( KM_SUCCESS(result) )
    m_Dirname = dirname;

  KM_RESULT_STATE_TEST_IMPLICIT();
  return result;
}

//
Result_t
Kumu::DirScannerEx::Close()
{
  if ( m_Handle == NULL )
    return RESULT_FILEOPEN;

  if ( closedir(m_Handle) == -1 )
    {
      switch ( errno )
	{
	case EBADF:
	case EINTR:
	  KM_RESULT_STATE_HERE();
	  return RESULT_STATE;

	default:
	  DefaultLogSink().Error("DirScanner::Close(): %s\n", strerror(errno));
	  return RESULT_FAIL;
	}
    }

  m_Handle = 0;
  return RESULT_OK;
}

//
Result_t
Kumu::DirScannerEx::GetNext(std::string& next_item_name, DirectoryEntryType_t& next_item_type)
{
  if ( m_Handle == 0 )
    return RESULT_FILEOPEN;

#if defined(__sun) && defined(__SVR4)
  struct stat s;
#endif
  struct dirent* entry;

  for (;;)
    {
      if ( ( entry = readdir(m_Handle) ) == 0 )
	return RESULT_ENDOFFILE;

      break;
    }

  next_item_name.assign(entry->d_name, strlen(entry->d_name));

#if defined(__sun) && defined(__SVR4)

  stat(entry->d_name, &s);

  switch ( s.st_mode )
    {
    case S_IFDIR:
      next_item_type = DET_DIR;
      break;

    case S_IFREG:
      next_item_type = DET_FILE;
      break;

    case S_IFLNK:
      next_item_type = DET_LINK;
      break;

    default:
      next_item_type = DET_DEV;
    }
#else // __sun 
  switch ( entry->d_type )
    {
    case DT_DIR:
      next_item_type = DET_DIR;
      break;

    case DT_REG:
      next_item_type = DET_FILE;
      break;

    case DT_LNK:
      next_item_type = DET_LINK;
      break;

    default:
      next_item_type = DET_DEV;
    }
#endif // __sun
  return RESULT_OK;
}

#endif // UNDEF // was: KM_WIN32


//------------------------------------------------------------------------------------------

//
// Attention Windows users: make sure to use the proper separator character
// with these functions.
//

// given a path string, create any missing directories so that PathIsDirectory(Path) is true.
//
Result_t
Kumu::CreateDirectoriesInPath(const std::string& Path)
{
  bool abs = PathIsAbsolute(Path);
  PathCompList_t PathComps, TmpPathComps;

  PathToComponents(Path, PathComps);

  while ( ! PathComps.empty() )
    {
      TmpPathComps.push_back(PathComps.front());
      PathComps.pop_front();
      std::string tmp_path = abs ? ComponentsToAbsolutePath(TmpPathComps) : ComponentsToPath(TmpPathComps);

      if ( ! PathIsDirectory(tmp_path) )
	{
#ifdef KM_WIN32
	  if ( _mkdir(tmp_path.c_str()) != 0 )
#else // KM_WIN32
	  if ( mkdir(tmp_path.c_str(), 0777) != 0 )
#endif // KM_WIN32
	    {
	      DefaultLogSink().Error("CreateDirectoriesInPath mkdir %s: %s\n",
				     tmp_path.c_str(), strerror(errno));
	      return RESULT_DIR_CREATE;
	    }
	}
    }

  return RESULT_OK;
}


//
Result_t
Kumu::DeleteFile(const std::string& filename)
{
  if ( _unlink(filename.c_str()) == 0 )
    return RESULT_OK;

  switch ( errno )
    {
    case ENOENT:
    case ENOTDIR: return RESULT_NOTAFILE;

    case EROFS:
    case EBUSY:
    case EACCES:
    case EPERM:   return RESULT_NO_PERM;
    }

  DefaultLogSink().Error("DeleteFile %s: %s\n", filename.c_str(), strerror(errno));
  return RESULT_FAIL;
}

//
Result_t
h__DeletePath(const std::string& pathname)
{
  if ( pathname.empty() )
    return RESULT_NULL_STR;

  Result_t result = RESULT_OK;

  if ( ! PathIsDirectory(pathname) )
    {
      result = DeleteFile(pathname);
    }
  else
    {
      {
	DirScanner TestDir;
	char       next_file[Kumu::MaxFilePath];
	result = TestDir.Open(pathname.c_str());

	while ( KM_SUCCESS(result) && KM_SUCCESS(TestDir.GetNext(next_file)) )
	  {
	    if ( next_file[0] == '.' )
	      {
		if ( next_file[1] ==  0 )
		  continue; // don't delete 'this'
		
		if ( next_file[1] == '.' && next_file[2] ==  0 )
		  continue; // don't delete 'this' parent
	      }

	    result = h__DeletePath(pathname + std::string("/") + next_file);
	  }
      }

      if ( _rmdir(pathname.c_str()) != 0 )
	{
	  switch ( errno )
	    {
	    case ENOENT:
	    case ENOTDIR:
	      result = RESULT_NOTAFILE;
	      break;

	    case EROFS:
	    case EBUSY:
	    case EACCES:
	    case EPERM:
	      result = RESULT_NO_PERM;
	      break;

	    default:
	      DefaultLogSink().Error("DeletePath %s: %s\n", pathname.c_str(), strerror(errno));
	      result = RESULT_FAIL;
	    }
	}
    }

  return result;
}

//
Result_t
Kumu::DeletePath(const std::string& pathname)
{
  std::string c_pathname = PathMakeCanonical(PathMakeAbsolute(pathname));
  DefaultLogSink().Debug("DeletePath (%s) c(%s)\n", pathname.c_str(), c_pathname.c_str());
  return h__DeletePath(c_pathname);
}


//
Result_t
Kumu::DeleteDirectoryIfEmpty(const std::string& path)
{
  DirScanner source_dir;
  char next_file[Kumu::MaxFilePath];

  Result_t result = source_dir.Open(path);

  if ( KM_FAILURE(result) )
    return result;

  while ( KM_SUCCESS(source_dir.GetNext(next_file)) )
    {
      if ( ( next_file[0] == '.' && next_file[1] == 0 )
	   || ( next_file[0] == '.' && next_file[1] == '.' && next_file[2] == 0 ) )
	continue;

      return RESULT_NOT_EMPTY; // anything other than "." and ".." indicates a non-empty directory
    }

  return DeletePath(path);
}


//------------------------------------------------------------------------------------------
//


Result_t
Kumu::FreeSpaceForPath(const std::string& path, Kumu::fsize_t& free_space, Kumu::fsize_t& total_space)
{
#ifdef KM_WIN32
  ULARGE_INTEGER lTotalNumberOfBytes;
  ULARGE_INTEGER lTotalNumberOfFreeBytes;

  BOOL fResult = ::GetDiskFreeSpaceExA(path.c_str(), NULL, &lTotalNumberOfBytes, &lTotalNumberOfFreeBytes);
  if ( fResult )
    {
      free_space = static_cast<Kumu::fsize_t>(lTotalNumberOfFreeBytes.QuadPart);
      total_space = static_cast<Kumu::fsize_t>(lTotalNumberOfBytes.QuadPart);
      return RESULT_OK;
    }

  HRESULT last_error = ::GetLastError();

  DefaultLogSink().Error("FreeSpaceForPath GetDiskFreeSpaceEx %s: %lu\n", path.c_str(), last_error);
  return RESULT_FAIL;
#else // KM_WIN32
  struct statfs s;

#if defined(__sun) && defined(__SVR4)
  if ( statfs(path.c_str(), &s, s.f_bsize, s.f_fstyp ) == 0 )
    {      if ( s.f_blocks < 1 )
	{
	  DefaultLogSink().Error("File system %s has impossible size: %ld\n",
				 path.c_str(), s.f_blocks);
	  return RESULT_FAIL;
	}

      free_space = (Kumu::fsize_t)s.f_bsize * (Kumu::fsize_t)s.f_bfree;
      total_space = (Kumu::fsize_t)s.f_bsize * (Kumu::fsize_t)s.f_blocks;
      return RESULT_OK;
    }
#else
  if ( statfs(path.c_str(), &s) == 0 )
    {
      if ( s.f_blocks < 1 )
	{
	  DefaultLogSink().Error("File system %s has impossible size: %ld\n",
				 path.c_str(), s.f_blocks);
	  return RESULT_FAIL;
	}

      free_space = (Kumu::fsize_t)s.f_bsize * (Kumu::fsize_t)s.f_bavail;
      total_space = (Kumu::fsize_t)s.f_bsize * (Kumu::fsize_t)s.f_blocks;
      return RESULT_OK;
    }

  switch ( errno )
    {
    case ENOENT:
    case ENOTDIR: return RESULT_NOTAFILE;
    case EACCES:  return RESULT_NO_PERM;
    }

  DefaultLogSink().Error("FreeSpaceForPath statfs %s: %s\n", path.c_str(), strerror(errno));
  return RESULT_FAIL;
#endif // __sun 
#endif // KM_WIN32
} 


//
// end KM_fileio.cpp
//
