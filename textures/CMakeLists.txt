function(DownloadIfNotPresent fileName srcUrl)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${fileName}")
        file(DOWNLOAD ${srcUrl} "${CMAKE_CURRENT_SOURCE_DIR}/${fileName}")
    endif()
endfunction(DownloadIfNotPresent)


## SUN ##
DownloadIfNotPresent("sun_albedo.jpg" https://www.solarsystemscope.com/textures/download/8k_sun.jpg)


## PLANETS ##

# Albedo
DownloadIfNotPresent("mercury_albedo.jpg" https://www.solarsystemscope.com/textures/download/8k_mercury.jpg)
DownloadIfNotPresent("venus_albedo.jpg" https://www.solarsystemscope.com/textures/download/8k_venus_surface.jpg)
DownloadIfNotPresent("earth_albedo.jpg" https://www.solarsystemscope.com/textures/download/8k_earth_daymap.jpg)
    DownloadIfNotPresent("moon_albedo.jpg" https://www.solarsystemscope.com/textures/download/8k_moon.jpg)
DownloadIfNotPresent("mars_albedo.jpg" https://www.solarsystemscope.com/textures/download/8k_mars.jpg)
DownloadIfNotPresent("jupiter_albedo.jpg" https://www.solarsystemscope.com/textures/download/8k_jupiter.jpg)
DownloadIfNotPresent("saturn_albedo.jpg" https://www.solarsystemscope.com/textures/download/8k_saturn.jpg)
DownloadIfNotPresent("uranus_albedo.jpg" https://www.solarsystemscope.com/textures/download/2k_uranus.jpg)
DownloadIfNotPresent("neptune_albedo.jpg" https://www.solarsystemscope.com/textures/download/2k_neptune.jpg)

# Atmosphere
#DownloadIfNotPresent("venus_atmosphere.jpg" https://www.solarsystemscope.com/textures/download/4k_venus_atmosphere.jpg)

# Rings
#DownloadIfNotPresent("saturn_rings.png" https://www.solarsystemscope.com/textures/download/8k_saturn_ring_alpha.png)


## BACKGROUND ##

# Background
DownloadIfNotPresent("background.jpg" https://www.solarsystemscope.com/textures/download/8k_stars_milky_way.jpg)
