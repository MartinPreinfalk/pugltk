#message("oat.cmake was included")

function(embed_ttf infile symbolname outfile)
    message("embed_ttf ${infile} ${symbolname} ${outfile}")
    add_custom_command(
        OUTPUT
            ${outfile}
        COMMAND imgui_ttf_font_binary_to_compressed_c -u8 ${infile} ${symbolname} > ${outfile}
        DEPENDS
            ${infile}
            imgui_ttf_font_binary_to_compressed_c
    )
endfunction()
