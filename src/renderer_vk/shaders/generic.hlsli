
#if PS
bool failsAlphaTest(uint alphaTest, float alpha){
    if(alphaTest == 1){
        if(alpha == 0){
            return true;
        }
    }
    if(alphaTest == 2){
        if(alpha >= 0.5){
            return true;
        }
    }
    if(alphaTest == 3){
        if(alpha < 0.5){
            return true;
        }
    }
    return false;
}

#endif


struct SceneView
{
  matrix projectionMatrix;
  float4 clipPlane;
};
[[vk::binding(2)]] ConstantBuffer<SceneView> sceneView;

float4 unpackColorR8G8B8A8(uint color){
    uint4 res = uint4(color, (color >> 8), (color >> 16), (color >> 24)) & uint4(0xFF, 0xFF, 0xFF, 0xFF);
    return float4(res)/255.0;
}

float4 unpackColorR11G11B10(uint color){
    uint4 res = uint4(color, (color >> 11), (color >> 22), 0xFF) & uint4(0x7FF, 0x7FF, 0x3FF, 0xFF);
    return float4(res)/float4(2047.0, 2047.0, 1023.0, 255.0);
}