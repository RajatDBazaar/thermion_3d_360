#include "ThermionFlutterWebApi.h"
#include "ResourceBuffer.hpp"

#include <thread>
#include <mutex>
#include <future>
#include <iostream>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/threading.h>
#include <emscripten/val.h>
#include <emscripten/fetch.h>

using emscripten::val;

extern "C"
{
  
  // 
  // Since are using -sMAIN_MODULE with -sPTHREAD_POOL_SIZE=1, main will be called when the first worker is spawned
  //
  
  // EMSCRIPTEN_KEEPALIVE int main() {
  //   std::cout << "WEBAPI MAIN " << std::endl;
  //   return 0;
  // }

  std::string _assetPathPrefix;

  EMSCRIPTEN_KEEPALIVE void thermion_dart_web_set_asset_path_prefix(const char* prefix) {
    _assetPathPrefix = std::string(prefix);
  }

  EMSCRIPTEN_KEEPALIVE EMSCRIPTEN_WEBGL_CONTEXT_HANDLE thermion_dart_web_create_gl_context() {

    EM_ASM(
      FS.mkdir('/indexed');
      FS.mount(IDBFS, {}, '/indexed');
      FS.syncfs(true, function (err) {
          if (err) {
              console.error('Error mounting IDBFS:', err);
          } else {
              console.log('IDBFS mounted successfully');
          }
      });
    );

    std::cout << "Creating WebGL context." << std::endl;

    EmscriptenWebGLContextAttributes attr;
    
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = EM_TRUE;
    attr.depth = EM_TRUE;
    attr.stencil = EM_FALSE;
    attr.antialias = EM_FALSE;
    attr.explicitSwapControl = EM_FALSE;
    attr.preserveDrawingBuffer = EM_TRUE;
    attr.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
    attr.enableExtensionsByDefault = EM_TRUE;
    attr.renderViaOffscreenBackBuffer = EM_FALSE;
    attr.majorVersion = 2;
    
    auto context = emscripten_webgl_create_context("#canvas", &attr);
    
    std::cout << "Created WebGL context " << attr.majorVersion << "." << attr.minorVersion << std::endl;

    auto success = emscripten_webgl_make_context_current((EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)context);
    if(success != EMSCRIPTEN_RESULT_SUCCESS) {
      std::cout << "Failed to make WebGL context current"<< std::endl;
    } else { 
      std::cout << "Made WebGL context current"<< std::endl;
      // glClearColor(0.0, 0.0, 1.0, 1.0);
      // glClear(GL_COLOR_BUFFER_BIT);
      // emscripten_webgl_commit_frame();
    }
    return context;
  }

  int _lastResourceId = 0;

  ResourceBuffer thermion_filament_web_load_resource(const char* path)
  {
    // ideally we should bounce the call to Flutter then wait for callback
    // this isn't working for large assets though - seems like it's deadlocked
    // will leave this here commented out so we can revisit later if needed
    // auto pendingCall = new PendingCall();
    // loadFlutterAsset(path, (void*)pendingCall);
    // pendingCall->Wait();
    // auto rb = ResourceBuffer { pendingCall->data, (int32_t) pendingCall->length, _lastResourceId  } ;
    _lastResourceId++;
    // delete pendingCall;
    // std::cout << "Deleted pending call" << std::endl;

    // emscripten_fetch_attr_t attr;
    // emscripten_fetch_attr_init(&attr);
    // attr.onsuccess = [](emscripten_fetch_t* fetch) {
      
    // };
    // attr.onerror = [](emscripten_fetch_t* fetch) {
    //   std::cout << "Error" << std::endl;
    // };
    // attr.onprogress = [](emscripten_fetch_t* fetch) {
      
    // };
    // attr.onreadystatechange = [](emscripten_fetch_t* fetch) {
      
    // };
    // attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    // const char* headers[] = {"Accept-Encoding", "gzip, deflate", NULL};
    // attr.requestHeaders = headers;

    // auto pathString = std::string(path);
    // if(pathString.rfind("/",0) != 0) {
    //   pathString = std::string("/") + pathString;
    // }
    
    // std::cout << "Fetching from path " << pathString.c_str() << std::endl;

    // auto request = emscripten_fetch(&attr, pathString.c_str());
    // if(!request) {
    //   std::cout << "Request failed?" << std::endl;  
    //   return ResourceBuffer { nullptr, 0, -1 } ;
    // }
    // auto data = malloc(request->numBytes);
    // memcpy(data, request->data, request->numBytes);
    // emscripten_fetch_close(request);
    // return ResourceBuffer { data, (int32_t) request->numBytes, _lastResourceId  } ;
    auto pathString = _assetPathPrefix + std::string(path);
    void* data = nullptr;
    int32_t numBytes = 0;

      
    // Check if the file exists in IndexedDB first
    bool fileExists = EM_ASM_INT({
        var filename = UTF8ToString($0);
        try {
            var stat = FS.stat('/indexed/' + filename);
            return stat.size > 0;
        } catch (e) {
            return false;
        }
    }, pathString.c_str());

    if (fileExists) {
        // File exists in IndexedDB, read it
        EM_ASM({
            var filename = UTF8ToString($0);
            var content = FS.readFile('/indexed/' + filename);
            var numBytes = content.length;
            var ptr = _malloc(numBytes);
            HEAPU8.set(content, ptr);
            setValue($1, ptr, 'i32');
            setValue($2, numBytes, 'i32');
        }, pathString.c_str(), &data, &numBytes);
    } else {
      void** pBuffer = (void**)malloc(sizeof(void*));
      int* pNum = (int*) malloc(sizeof(int*));
      int* pError = (int*)malloc(sizeof(int*));
      emscripten_wget_data(pathString.c_str(), pBuffer, pNum, pError);
      data = *pBuffer;
      numBytes = *pNum;

      // Save the file to IndexedDB filesystem
      EM_ASM({
          var filename = UTF8ToString($0);
          var data = new Uint8Array(HEAPU8.subarray($1, $1 + $2));

          // Ensure the '/indexed' directory exists
          if (!FS.analyzePath('/indexed').exists) {
              FS.mkdir('/indexed');
          }
                
          // Create all parent directories
          var parts = filename.split('/');
          var currentPath = '/indexed';
          for (var i = 0; i < parts.length - 1; i++) {
              currentPath += '/' + parts[i];
              if (!FS.analyzePath(currentPath).exists) {
                  FS.mkdir(currentPath);
              }
          }
          // Write the file
          FS.writeFile('/indexed/' + filename, data);
          
          FS.syncfs(false, function (err) {
              if (err) {
                  console.error('Failed to save file to IndexedDB:', err);
              } else {
                  console.log('File saved to IndexedDB successfully');
              }
          });
      }, pathString.c_str(), data, numBytes);

      free(pBuffer);
      free(pNum);
      free(pError);
    }
    return ResourceBuffer { data, numBytes, _lastResourceId  } ;   
  }

  void thermion_filament_web_free_resource(ResourceBuffer rb) {
    free((void*)rb.data);
  }
  
  EMSCRIPTEN_KEEPALIVE void thermion_filament_web_free(void* ptr) {
    free(ptr);
  }

  EMSCRIPTEN_KEEPALIVE void* thermion_dart_web_get_resource_loader_wrapper() {
    ResourceLoaderWrapper *rlw = (ResourceLoaderWrapper *)malloc(sizeof(ResourceLoaderWrapper));
    rlw->loadResource = thermion_filament_web_load_resource;
    rlw->loadFromOwner = nullptr;
    rlw->freeResource = thermion_filament_web_free_resource;
    rlw->freeFromOwner = nullptr;
    rlw->loadToOut = nullptr;
    rlw->owner = nullptr;
    return rlw;
  }
}