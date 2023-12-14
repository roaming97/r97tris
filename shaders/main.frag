#version 430 core
out vec4 fragColor;
in vec3 ourColor;
in vec2 texCoord;
uniform float iTime;
uniform vec3 iResolution;
uniform sampler2D fgTex;
uniform sampler2D bgTex;

vec4 layer(vec4 foreground,vec4 background){
    return foreground*foreground.a+background*(1.-foreground.a);
}

float map(vec3 p){
    
    vec3 q=sin(p*1.9);
    
    float w1=4.-abs(p.y)+(q.x+q.z)*.8;
    float w2=4.-abs(p.x)+(q.y+q.z)*.8;
    
    float s1=length(mod(p.xy+vec2(sin((p.z+p.x)*2.)*.25,cos((p.z+p.x)*1.)*.5),2.)-1.)-.2;
    float s2=length(mod(.5+p.yz+vec2(sin((p.z+p.x)*2.)*.25,cos((p.z+p.x)*1.)*.3),2.)-1.)-.2;
    
    return min(w1,min(w2,min(s1,s2)));
    
}

vec2 rot(vec2 p,float a){
    return vec2(
        p.x*cos(a)-p.y*sin(a),
        p.x*sin(a)+p.y*cos(a));
    }
    
    void main()
    {
        vec2 fromcenter=texCoord-vec2(.5,.5);
        float ratio=1;
        if(iResolution.x>=720.&&iResolution.y>=640.){
            ratio=1.5;
        }
        if(iResolution.x>=1000.&&iResolution.y>=960.){
            ratio=2.;
        }
        vec2 scale=vec2(1./(640.*ratio/iResolution.x),1./(480.*ratio/iResolution.y));
        vec2 scaled=fromcenter*scale;
        vec2 resultc=vec2(.5,.5)+scaled;
        
        vec4 bg=texture(bgTex,resultc);
        vec4 fg=texture(fgTex,resultc);
        
        vec2 uv=(2.*gl_FragCoord.xy-iResolution.xy)/iResolution.y;
        vec3 dir=normalize(vec3(uv,1.));
        dir.xz=rot(dir.xz,iTime*.23);dir=dir.yzx;
        dir.xz=rot(dir.xz,iTime*.2);dir=dir.yzx;
        vec3 pos=vec3(0,0,iTime);
        vec3 col=vec3(0.);
        
        float t=0.,tt;
        
        for(int i=0;i<100;i++){
            tt=map(pos+dir*t);
            if(abs(tt)<.003)break;
            t+=tt*.7;
        }
        
        vec3 ip=pos+dir*t;
        col=vec3(t*.1);
        col=sqrt(col);
        vec4 shader=vec4(.05*t+abs(dir)*col+max(0.,map(ip-.1)-tt),1.);
        
        fragColor=mix(shader,fg,.95);
    }