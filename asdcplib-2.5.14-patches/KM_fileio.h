/*
Copyright (c) 2004-2014, John Hurst
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
  /*! \file    KM_fileio.h
    \version $Id: KM_fileio.h,v 1.22 2015/10/07 16:41:23 jhurst Exp $
    \brief   portable file i/o
  */

#ifndef _KM_FILEIO_H_
#define _KM_FILEIO_H_

#include <KM_util.h>
#include <string>

#ifdef KM_WIN32
# include <io.h>
# include "dirent_win.h"
#else
# include <dirent.h>
# include <unistd.h>
# include <time.h>
# include <sys/types.h>
#include <regex.h>
#endif

#include <sys/stat.h>



namespace Kumu
{
  //
  class DirScanner
    {
    public:
#ifdef UNDEF // was: _KM_WIN32
      __int64               m_Handle;
      struct _finddatai64_t m_FileInfo;
#else
      DIR*       m_Handle;
#endif

      DirScanner(void);
      ~DirScanner() { Close(); }

      Result_t Open(const std::string&);
      Result_t Close();
      Result_t GetNext(char*);
    };


  // 
  enum DirectoryEntryType_t {
    DET_FILE,
    DET_DIR,
    DET_DEV,
    DET_LINK
  };

  //
  class DirScannerEx
  {
    std::string m_Dirname;
#ifdef _KM_WIN32
    __int64               m_Handle;
    struct _finddatai64_t m_FileInfo;
#else
    DIR*       m_Handle;
#endif

    KM_NO_COPY_CONSTRUCT(DirScannerEx);

  public:
    
    DirScannerEx();
    ~DirScannerEx() { Close(); }

    Result_t Open(const std::string& dirname);
    Result_t Close();


    inline Result_t GetNext(std::string& next_item_name) {
      DirectoryEntryType_t ft;
      return GetNext(next_item_name, ft);
    }

    Result_t GetNext(std::string& next_item_name, DirectoryEntryType_t& next_item_type);
  };

#ifdef KM_WIN32
  typedef __int64  fsize_t;
  typedef __int64  fpos_t;
  typedef HANDLE FileHandle;

  enum SeekPos_t {
    SP_BEGIN = FILE_BEGIN,
    SP_POS   = FILE_CURRENT,
    SP_END   = FILE_END
  };
#else
  typedef off_t    fsize_t;
  typedef off_t    fpos_t;
  typedef int      FileHandle;
  const FileHandle INVALID_HANDLE_VALUE = -1L;

  enum SeekPos_t {
    SP_BEGIN = SEEK_SET,
    SP_POS   = SEEK_CUR,
    SP_END   = SEEK_END
  };
#endif

  //
#ifndef KM_SMALL_FILES_OK
  template <bool sizecheck>    void compile_time_size_checker();
  template <> inline void compile_time_size_checker<false>() {}
  //
  // READ THIS if your compiler is complaining about a previously declared implementation of
  // compile_time_size_checker(). For example, GCC 4.0.1 looks like this:
  //
  // error: 'void Kumu::compile_time_size_checker() [with bool sizecheck = false]' previously declared here
  //
  // This is happening because the equality being tested below is false. The reason for this 
  // will depend on your OS, but on Linux it is probably because you have not used -D_FILE_OFFSET_BITS=64
  // Adding this magic macro to your CFLAGS will get you going again. If you are on a system that
  // does not support 64-bit files, you can disable this check by using -DKM_SMALL_FILES_OK. You
  // will then of course be limited to file sizes < 4GB.
  //
  template <> inline void compile_time_size_checker<sizeof(Kumu::fsize_t)==sizeof(ui64_t)>() {}
#endif
  //

  const ui32_t Kilobyte = 1024;
  const ui32_t Megabyte = Kilobyte * Kilobyte;
  const ui32_t Gigabyte = Megabyte * Kilobyte;

  const ui32_t MaxFilePath = Kilobyte;


  //------------------------------------------------------------------------------------------
  // Path Manglers
  //------------------------------------------------------------------------------------------

  // types
  typedef std::list<std::string> PathCompList_t; // a list of path components
  typedef std::list<std::string> PathList_t; // a list of paths

  // tests
  bool        PathExists(const std::string& Path); // true if the path exists in the filesystem
  bool        PathIsFile(const std::string& Path); // true if the path exists in the filesystem and is a file
  bool        PathIsDirectory(const std::string& Path); // true if the path exists in the filesystem and is a directory
  fsize_t     FileSize(const std::string& Path); // returns the size of a regular file, 0 for a directory or device
  std::string PathCwd();
  bool        PathsAreEquivalent(const std::string& lhs, const std::string& rhs); // true if paths point to the same filesystem entry

  // Returns free space and total space available for the given path
  Result_t    FreeSpaceForPath(const std::string& path, Kumu::fsize_t& free_space, Kumu::fsize_t& total_space);

  // split and reassemble paths as lists of path components
  PathCompList_t& PathToComponents(const std::string& Path, PathCompList_t& CList, char separator = '/'); // removes '//'
  std::string ComponentsToPath(const PathCompList_t& CList, char separator = '/');
  std::string ComponentsToAbsolutePath(const PathCompList_t& CList, char separator = '/'); // add separator to the front
  bool        PathHasComponents(const std::string& Path, char separator = '/'); // true if paths starts with separator

  bool        PathIsAbsolute(const std::string& Path, char separator = '/'); // true if path begins with separator
  std::string PathMakeAbsolute(const std::string& Path, char separator = '/'); // compute position of relative path using getcwd()
  std::string PathMakeLocal(const std::string& Path, const std::string& Parent); // remove Parent from front of Path, if it exists
  std::string PathMakeCanonical(const std::string& Path, char separator = '/'); // remove '.' and '..'
  bool        PathResolveLinks(const std::string& link_path, std::string& resolved_path, char separator = '/');

  // common operations
  std::string PathBasename(const std::string& Path, char separator = '/'); // returns right-most path element (list back())
  std::string PathDirname(const std::string& Path, char separator = '/'); // returns everything but the right-most element
  std::string PathGetExtension(const std::string& Path); // returns everything in the right-most element following the right-most '.'
  std::string PathSetExtension(const std::string& Path, const std::string& Extension); // empty extension removes '.' as well

  std::string PathJoin(const std::string& Path1, const std::string& Path2, char separator = '/');
  std::string PathJoin(const std::string& Path1, const std::string& Path2, const std::string& Path3, char separator = '/');
  std::string PathJoin(const std::string& Path1, const std::string& Path2,
		       const std::string& Path3, const std::string& Path4, char separator = '/');


  //------------------------------------------------------------------------------------------
  // Path Search
  //------------------------------------------------------------------------------------------

  // An interface for a path matching function, used by FindInPath() and FindInPaths() below
  //
  class IPathMatch
  {
  public:
    virtual ~IPathMatch() {}
    virtual bool Match(const std::string& s) const = 0;
  };

  // matches any pathname
 class PathMatchAny : public IPathMatch
  {
  public:
    virtual ~PathMatchAny() {}
    inline bool Match(const std::string&) const { return true; }
  };

#ifndef KM_WIN32
  // matches pathnames using a regular expression
 class PathMatchRegex : public IPathMatch
  {
    regex_t m_regex;
    PathMatchRegex();
    const PathMatchRegex& operator=(const PathMatchRegex&);

  public:
    PathMatchRegex(const std::string& Pattern);
    PathMatchRegex(const PathMatchRegex&);
    virtual ~PathMatchRegex();
    bool Match(const std::string& s) const;
  };

  // matches pathnames using a Bourne shell glob expression
 class PathMatchGlob : public IPathMatch
  {
    regex_t m_regex;
    PathMatchGlob();
    const PathMatchGlob& operator=(const PathMatchGlob&);

  public:
    PathMatchGlob(const std::string& Pattern);
    PathMatchGlob(const PathMatchGlob&);
    virtual ~PathMatchGlob();
    bool Match(const std::string& s) const;
  };
#endif /* !KM_WIN32 */

  // Search all paths in SearchPaths for filenames matching Pattern (no directories are returned).
  // Put results in FoundPaths. Returns after first find if one_shot is true.
  PathList_t& FindInPath(const IPathMatch& Pattern, const std::string& SearchDir,
			 PathList_t& FoundPaths, bool one_shot = false, char separator = '/');

  PathList_t& FindInPaths(const IPathMatch& Pattern, const PathList_t& SearchPaths,
			  PathList_t& FoundPaths, bool one_shot = false, char separator = '/');

  std::string GetExecutablePath(const std::string& default_path);

  //------------------------------------------------------------------------------------------
  // Directory Manipulation
  //------------------------------------------------------------------------------------------

  // Create a directory, creates intermediate directories as necessary
  Result_t CreateDirectoriesInPath(const std::string& Path);

  // Delete a file (fails if the path points to a directory)
  Result_t DeleteFile(const std::string& filename);

  // Recursively remove a file or directory
  Result_t DeletePath(const std::string& pathname);

  // Remove the path only if it is a directory that is empty.
  Result_t DeleteDirectoryIfEmpty(const std::string& path);

  //------------------------------------------------------------------------------------------
  // File I/O Wrappers
  //------------------------------------------------------------------------------------------

  // Instant IO for strings
  //
  // Reads an entire file into a string.
  Result_t ReadFileIntoString(const std::string& filename, std::string& outString, ui32_t max_size = 8 * Megabyte);

  // Writes a string to a file, overwrites the existing file if present.
  Result_t WriteStringIntoFile(const std::string& filename, const std::string& inString);

  // Instant IO for archivable objects
  //
  // Unarchives a file into an object
  Result_t ReadFileIntoObject(const std::string& Filename, IArchive& Object, ui32_t max_size = 8 * Kumu::Megabyte);

  // Archives an object into a file
  Result_t WriteObjectIntoFile(const IArchive& Object, const std::string& Filename);

  // Instant IO for memory buffers
  //
  // Unarchives a file into a buffer
  Result_t ReadFileIntoBuffer(const std::string& Filename, Kumu::ByteString& Buffer,
			      ui32_t max_size = 8 * Kumu::Megabyte);

  // Archives a buffer into a file
  Result_t WriteBufferIntoFile(const Kumu::ByteString& Buffer, const std::string& Filename);


  //------------------------------------------------------------------------------------------
  // File I/O
  //------------------------------------------------------------------------------------------

  //
  class FileReader
    {
      KM_NO_COPY_CONSTRUCT(FileReader);

    protected:
      std::string m_Filename;
      FileHandle  m_Handle;

    public:
      FileReader() : m_Handle(INVALID_HANDLE_VALUE) {}
      virtual ~FileReader() { Close(); }

      Result_t OpenRead(const std::string&) const;                          // open the file for reading
      Result_t Close() const;                                        // close the file
      fsize_t  Size() const;                                         // returns the file's current size
      Result_t Seek(Kumu::fpos_t = 0, SeekPos_t = SP_BEGIN) const;   // move the file pointer
      Result_t Tell(Kumu::fpos_t* pos) const;                        // report the file pointer's location
      Result_t Read(byte_t*, ui32_t, ui32_t* = 0) const;             // read a buffer of data

      inline Kumu::fpos_t Tell() const                               // report the file pointer's location
	{
	  Kumu::fpos_t tmp_pos;
	  Tell(&tmp_pos);
	  return tmp_pos;
	}

      inline bool IsOpen() {                                         // returns true if the file is open
	return (m_Handle != INVALID_HANDLE_VALUE);
      }
    };

  //
  class FileWriter : public FileReader
    {
      class h__iovec;
      mem_ptr<h__iovec>  m_IOVec;
      KM_NO_COPY_CONSTRUCT(FileWriter);

    public:
      FileWriter();
      virtual ~FileWriter();

      Result_t OpenWrite(const std::string&);                               // open a new file, overwrites existing
      Result_t OpenModify(const std::string&);                              // open a file for read/write

      // this part of the interface takes advantage of the iovec structure on
      // platforms that support it. For each call to Writev(const byte_t*, ui32_t, ui32_t*),
      // the given buffer is added to an internal iovec struct. All items on the list
      // are written to disk by a call to Writev();
      Result_t Writev(const byte_t*, ui32_t);                       // queue buffer for "gather" write
      Result_t Writev(ui32_t* = 0);                                 // write all queued buffers

      // if you call this while there are unwritten items on the iovec list,
      // the iovec list will be written to disk before the given buffer,as though
      // you had called Writev() first.
      Result_t Write(const byte_t*, ui32_t, ui32_t* = 0);            // write buffer to disk
   };

  Result_t CreateDirectoriesInPath(const std::string& Path);
  Result_t FreeSpaceForPath(const std::string& path, Kumu::fsize_t& free_space, Kumu::fsize_t& total_space);
  Result_t DeleteFile(const std::string& filename);
  Result_t DeletePath(const std::string& pathname);

} // namespace Kumu


#endif // _KM_FILEIO_H_


//
// end KM_fileio.h
//
