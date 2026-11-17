in vec3 colour;
out vec4 frag_colour;
uniform int u_style_choice;

void main()
{

    if(u_style_choice == 0)
    {
        frag_colour = vec4(colour, 1.0);
        



    }

    if(u_style_choice == 1)
    {
        frag_colour = vec4(1.0f,0.0f,0.0f, 1.0);
        



    }


    
}