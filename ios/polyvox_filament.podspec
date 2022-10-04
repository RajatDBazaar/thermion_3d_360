#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint polyvox_filament.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'polyvox_filament'
  s.version          = '0.0.1'
  s.summary          = 'A new flutter plugin project.'
  s.description      = <<-DESC
A new flutter plugin project.
                       DESC
  s.homepage         = 'http://example.com'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'Your Company' => 'email@example.com' }
  s.source           = { :path => '.' }
  s.source_files = 'src/*', 'src/ios/*', 'src/shaders/*.c', 'src/shaders/*.h', 'include/filament/*', 'include/*', 'include/material/*.h', 'include/material/*.c'
  s.public_header_files = 'include/SwiftPolyvoxFilamentPlugin-Bridging-Header.h',  'include/PolyvoxFilamentIOSApi.h', 'include/PolyvoxFilamentApi.h', 'include/ResourceBuffer.hpp' #, 'include/filament/*'
#  s.header_mappings_dir = 'include'
  s.dependency 'Flutter' 
  s.platform = :ios, '12.1'
  s.static_framework = true
  s.vendored_libraries = "lib/*.a"

  s.library = "c++"
  s.user_target_xcconfig = { 
    'DEFINES_MODULE' => 'YES', 
    'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386', 
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++17",
    'OTHER_CXXFLAGS' => '"--std=c++17" "-fmodules" "-fcxx-modules" "$(inherited)"',
    'USER_HEADER_SEARCH_PATHS' => '"${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/include" "${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/src" "${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/src/image" "${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/src/shaders"  "$(inherited)"',
    'ALWAYS_SEARCH_USER_PATHS' => 'YES',
    "OTHER_LDFLAGS" =>  '-lfilament -lbackend -lfilameshio -lviewer -lfilamat -lgeometry -lutils -lfilabridge -lgltfio_core -lfilament-iblprefilter -limage -lcamutils -lgltfio_core -lfilaflat -ldracodec -libl -lktxreader -limageio -lpng -lpng16 -ltinyexr  -lz -lstb -luberzlib -lsmol-v -luberarchive -lzstd',
    'LIBRARY_SEARCH_PATHS' => '"${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/lib" "$(inherited)"',
  }

  s.pod_target_xcconfig = { 
    'DEFINES_MODULE' => 'YES', 
    'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386', 
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++17",
    'OTHER_CXXFLAGS' => '"--std=c++17" "-fmodules" "-fcxx-modules" "$(inherited)"',
    'USER_HEADER_SEARCH_PATHS' => '"${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/include" "${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/src" "${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/src/image" "${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/src/shaders"  "$(inherited)"',
    'ALWAYS_SEARCH_USER_PATHS' => 'YES',
    "OTHER_LDFLAGS" =>  '-lfilament -lbackend -lfilameshio -lviewer -lfilamat -lgeometry -lutils -lfilabridge -lgltfio_core -lfilament-iblprefilter -limage -lcamutils -lgltfio_core -lfilaflat -ldracodec -libl -lktxreader -limageio -lpng -lpng16 -ltinyexr  -lz -lstb -luberzlib -lsmol-v -luberarchive -lzstd',
    'LIBRARY_SEARCH_PATHS' => '"${PODS_ROOT}/../.symlinks/plugins/polyvox_filament/ios/lib" "$(inherited)"',
  }

  s.swift_version = '5.0'
end
