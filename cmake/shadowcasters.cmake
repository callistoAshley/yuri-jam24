# FIXME: only make this run once
add_custom_target(shadowcasters ALL)
function(add_shadowcaster target)
  if (${ARGC} EQUAL 1)
    add_custom_command(
      TARGET shadowcasters
      COMMENT ${target}.shdw
      DEPENDS ${CMAKE_SOURCE_DIR}/assets/textures/${target}.png ${SHADOWCAST_NAME}
      #OUTPUT ${CMAKE_SOURCE_DIR}/assets/shadowcasters/${target}.shdw
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${SHADOWCAST_NAME} ${CMAKE_SOURCE_DIR}/assets/textures/${target}.png ${CMAKE_SOURCE_DIR}/assets/shadowcasters/${target}.shdw
      COMMAND echo "Generated shadowcaster for ${target}"
    )
  else()
    add_custom_command(
      TARGET shadowcasters
      COMMENT ${target}.shdw
      DEPENDS ${CMAKE_SOURCE_DIR}/assets/textures/${target}.png ${SHADOWCAST_NAME}
      #OUTPUT ${CMAKE_SOURCE_DIR}/assets/shadowcasters/${target}.shdw
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${SHADOWCAST_NAME} ${CMAKE_SOURCE_DIR}/assets/textures/${target}.png ${CMAKE_SOURCE_DIR}/assets/shadowcasters/${target}.shdw ${ARGV1} ${ARGV2}
      COMMAND echo "Generated shadowcaster for ${target}"
    )
  endif()
  message("Adding shadowcaster for ${target}")
  list(APPEND SHADOWCASTERS ${CMAKE_SOURCE_DIR}/assets/shadowcasters/${target}.shdw)
endfunction()

add_shadowcaster(player)
add_shadowcaster(red_start 8 8)
