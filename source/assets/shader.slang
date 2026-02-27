struct vertex_input
{
    float3 Pos;
    float3 Normal;
    float2 UV;
};

Sampler2D textures[];

struct shader_data
{
    float4x4 projection;
    float4x4 view;
    float4x4 model[3];
    float4 light_pos;
    uint32_t selected;
};

struct vertex_output
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
vertex_output main(vertex_input input, uniform shader_data *sh_data, uint instance_index: SV_VulkanInstanceID)
{
    vertex_output output;
    float4x4 model_mat = sh_data->model[instance_index];
    output.Normal = mul((float3x3)mul(sh_data->view, model_mat), input.Normal);
    output.UV = input.UV;
    output.Pos = mul(sh_data->projection, mul(sh_data->view, mul(model_mat, float4(input.Pos.xyz, 1.0))));
    output.Factor = (sh_data->selected == instance_index ? 3.0f : 1.0f);
    output.Instance_index = instance_index;
    float4 frag_pos = mul(mul(sh_data->view, model_mat), float4(input.Pos.xyz, 1.0));
    output.Light_vec = sh_data->light_pos.xyz - frag_pos.xyz;
    output.View_vec = -frag_pos.xyz;
    return output;
}

[shader("fragment")]
float4 main(vertex_output input)
{
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.Light_vec);
    float3 V = normalize(input.View_vec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0025);
    float3 specular = pow(max(dot(R, V), 0.0), 10.0) * 0.75;
    float3 color = textures[NonUniformResourceIndex(input.Instance_index)].Sample(input.UV).rgb * input.Factor;
    return float4(diffuse * color.rgb + specular, 1.0);
}