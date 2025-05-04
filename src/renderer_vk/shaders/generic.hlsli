
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


struct DynamicLight
{
    float3 position;
    float radius;
    float4 color;
};

struct SceneView
{
  matrix projectionMatrix;
  float4 clipPlane;
  DynamicLight lights[32];
  uint lightCount;
};
[[vk::binding(2)]] ConstantBuffer<SceneView> sceneView;

float4 unpackColor(uint color){
    uint4 res = uint4(color, (color >> 8),(color >> 16), (color >> 24)) & uint4(0xFF, 0xFF, 0xFF, 0xFF);
    return float4(res)/255.0;
}