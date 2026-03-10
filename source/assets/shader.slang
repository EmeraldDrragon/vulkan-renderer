[[vk::binding(0, 1)]]
Sampler2D textures[];
struct SceneData
{
    float4x4 projection;
    float4x4 view;
    float4 light_pos;
    uint32_t selected_instance;
};
struct PushConstants
{
    float4x4 model_mat;
    uint32_t texture_index;
    uint32_t instance_id;
}
[[vk::push_constant]] PushConstants pc;


struct vertexInput
{
    float3 Pos;
    float3 Normal;
    float2 UV;
};
struct vertexOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    float3 Factor;
    float3 Light_vec;
    float3 View_vec;
    uint32_t Instance_index;
};



[shader("vertex")]
vertexOutput vertexMain(vertexInput input, uniform SceneData* scene)
{
    vertexOutput output;

    float4x4 mvp = mul(scene->projection, mul(scene->view, pc.model_mat));
    output.Pos = mul(mvp, float4(input.Pos, 1.0));

    float4x4 model_view = mul(scene->view, pc.model_mat);
    output.Normal = mul((float3x3)model_view, input.Normal);

    float4 frag_pos = mul(model_view, float4(input.Pos, 1.0));
    output.Light_vec = scene->light_pos.xyz - frag_pos.xyz;
    output.View_vec = -frag_pos.xyz;

    output.UV = input.UV;

    output.Factor = (scene->selected_instance == pc.instance_id ? 3.0f : 1.0f);

    return output;
}

[shader("fragment")]
float4 fragmentMain(vertexOutput input)
{
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.Light_vec);
    float3 V = normalize(input.View_vec);
    float3 R = reflect(-L, N);

    float3 diffuse = max(dot(N, L), 0.05);
    float3 specular = pow(max(dot(R, V), 0.0), 10.0) * 0.75;
    float3 tex_color = textures[NonUniformResourceIndex(pc.texture_index)].Sample(input.UV).rgb;
    float3 color = (diffuse * tex_color + specular) * input.Factor;

    return float4(color, 1.0);
}