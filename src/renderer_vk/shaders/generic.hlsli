
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

#if VS

struct SceneView
{
  matrix projectionMatrix;
  float4 clipPlane;
};
[[vk::binding(2)]] ConstantBuffer<SceneView> sceneView;

#endif