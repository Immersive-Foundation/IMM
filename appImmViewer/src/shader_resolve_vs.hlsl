void main(uint vertexID : SV_VertexID,
	      out float4 oPosition : SV_Position )
{
    float x = -1.0 + 2.0*float(vertexID &1);
    float y = -1.0 + 2.0*float(vertexID>>1);
	oPosition = float4(x, y, 0.0, 1.0);

}
