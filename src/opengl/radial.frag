// Generated by src/gui/util/./cgtoinclude.sh from radial.cg
"!!ARBfp1.0"
"PARAM c[5] = { program.local[0..2],"
"{ 2, 4 },"
"program.local[4] };"
"TEMP R0;"
"MUL R0.zw, c[0].xyxy, fragment.texcoord[0].xyxy;"
"ADD R0.z, R0, R0.w;"
"MUL R0.xy, c[0].zwzw, fragment.texcoord[0];"
"ADD R0.w, R0.x, R0.y;"
"ADD R0.xy, R0.zwzw, c[1];"
"MUL R0.zw, R0.xyxy, R0.xyxy;"
"MUL R0.xy, R0, c[2];"
"ADD R0.x, R0, R0.y;"
"ADD R0.z, R0, R0.w;"
"MUL R0.z, c[4].x, -R0;"
"MUL R0.y, R0.z, c[3];"
"MUL R0.x, R0, c[3];"
"MAD R0.y, R0.x, R0.x, -R0;"
"MOV R0.z, c[3].x;"
"RSQ R0.y, R0.y;"
"MUL R0.z, c[4].x, R0;"
"RCP R0.y, R0.y;"
"RCP R0.z, R0.z;"
"ADD R0.x, -R0, R0.y;"
"MUL R0.x, R0, R0.z;"
"TEX result.color, R0, texture[0], 1D;"
"END"
; // Generated by src/gui/util/./cgtoinclude.sh from radial.cg
