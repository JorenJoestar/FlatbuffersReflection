#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <SDL.h>
#include <GL/glew.h>

// Flatbuffers generated classes
#include "RenderDefinitions_generated.h"
#include "flatbuffers/minireflect.h"

// Flatbuffers full reflection header
#include "flatbuffers/reflection.h"

#define ArrayLength(array) ( sizeof(array)/sizeof((array)[0]) )

// Table that allocates memory to be used mainly for UI editing.
struct FlatBuffersReflectionTable {

    std::vector<uint8_t>            data;
    std::vector<uint16_t>           member_offsets;
    const flatbuffers::TypeTable*   type_table;

    bool                            populated = false;

    void* GetData( uint32_t member_index ) const {
        return (member_index < type_table->num_elems) ? (void*)&data[member_offsets[member_index]] : nullptr;
    }

    void Populate( const flatbuffers::TypeTable* type_table ) {

        if ( populated ) {
            return;
        }
        this->type_table = type_table;
        populated = true;

        using namespace flatbuffers;

        for ( uint32_t i = 0; i < type_table->num_elems; ++i ) {
            const flatbuffers::TypeCode& type_code = type_table->type_codes[i];

            member_offsets.push_back( (uint16_t)data.size() );

            if ( type_code.base_type == flatbuffers::ET_SEQUENCE ) {
                // TODO: check for subtype
            }
            else {
                const size_t type_size = InlineSize( static_cast<ElementaryType>(type_code.base_type), type_table );

                for ( size_t s = 0; s < type_size; ++s )
                    data.push_back( 0 );

                if ( type_code.sequence_ref == 0 ) {
                    // Allocate always 4 bytes
                    const int32_t remaining_bytes = 4 - type_size;
                    for ( size_t s = 0; s < remaining_bytes; ++s )
                        data.push_back( 0 );
                }
            }
        }
    }
};

// Useful to see that a field has a type and a base_type.
// Taken from: https://groups.google.com/forum/#!topic/flatbuffers/nAi8MQu3A-U
void RecursiveReflection( const flatbuffers::Table* table, const reflection::Object* reflectionTable, const reflection::Schema* schema ) {
    if ( table == nullptr ) return;
    auto tableFields = reflectionTable->fields();
    for ( size_t i = 0; i < tableFields->Length(); i++ ) {
        if ( tableFields->Get( i )->type()->base_type() == reflection::Obj ) {
            flatbuffers::Table* t = flatbuffers::GetFieldT( *table, *tableFields->Get( i ) );
            const reflection::Object* refT = schema->objects()->Get( tableFields->Get( i )->type()->index() );

            RecursiveReflection( t, refT, schema );
        }
        std::string fieldName = tableFields->Get( i )->name()->c_str();
        auto fieldVal = flatbuffers::GetAnyFieldS( *table, *tableFields->Get( i ), schema );
    }
}

// Mini reflection building
// ..\bin\flatc --cpp ..\data\RenderDefinitions.fbs --reflect-names
// Full reflection building
// ..\bin\flatc.exe -b --schema reflection.fbs RenderDefinitions.fbs

static void ReflectUIFull() {

    using namespace flatbuffers;

    ImGui::Separator();
    ImGui::Text( "Flatbuffers UI Full Reflection" );
    ImGui::Separator();
    ImGui::NewLine();

    // 1. Obtain the schema from the binary fbs generated with reflection.fbs (very elegant solution!)
    std::string bfbsfile;
    bool found = flatbuffers::LoadFile( "..\\data\\RenderDefinitions.bfbs", true, &bfbsfile );

    const reflection::Schema& schema = *reflection::GetSchema( bfbsfile.c_str() );

    // 2. The root table contains all the types as objects.
    const reflection::Object* root_table = schema.root_table();

    ImGui::LabelText( "Root type", "%s", root_table->name()->c_str() );
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    ImGui::Text( "Types in the .fbs file:" );

    // 2.1. List all the types present in the fbs.
    auto types = schema.objects();
    for ( size_t i = 0; i < types->Length(); i++ ) {
        const reflection::Object* type = types->Get( i );
        ImGui::Text( "    %s", type->name()->c_str() );
    }

    ImGui::Text( "Enums in the .fbs file:" );
    // 3. Get all the enums
    auto enums = schema.enums();
    for ( size_t i = 0; i < enums->Length(); i++ ) {
        const reflection::Enum* enum_ = enums->Get( i );
        ImGui::Text( "    %s", enum_->name()->c_str() );
    }

    // 3.1. Being all data-driven, enums values need to be cached.
    //      For this example just cache the TextureFormats enum.
    static std::vector<std::string> s_texture_formats_enum_data;
    static const char** s_texture_formats_enum_values;
    if ( s_texture_formats_enum_data.size() == 0 ) {
        
        // Search for enum based on name (inefficient).
        const reflection::Enum* enum_ = nullptr;

        for ( size_t i = 0; i < enums->Length(); i++ ) {
            enum_ = enums->Get( i );
            if ( strcmp( enum_->name()->c_str(), "rendering.TextureFormat" ) == 0 ) {
                break;
            }
        }

        // Lazy initialization.
        if ( enum_ != nullptr ) {
            auto values = enum_->values();

            s_texture_formats_enum_data.reserve( values->Length() );

            s_texture_formats_enum_values = new const char*[values->Length()];

            for ( size_t v = 0; v < values->Length(); ++v ) {
                auto enum_value = values->Get( v );
                s_texture_formats_enum_data.push_back( enum_value->name()->str() );
                s_texture_formats_enum_values[v] = s_texture_formats_enum_data.back().c_str();
            }
        }
    }

    // 4. Get a specific type (notice the full namespace.type name).
    auto render_target_type = types->LookupByKey( "rendering.RenderTarget" );

    ImGui::Separator();
    ImGui::NewLine();

    ImGui::Text( "Fields for type %s", render_target_type->name()->c_str() );

    static char s_string_buffer[128];

    // 5. List all the fields of the type. Here we can create the custom UI based on annotation.
    auto fields = render_target_type->fields();
    if ( fields ) {

        // 5.1. List all the fields
        for ( size_t i = 0; i < fields->Length(); i++ ) {
            auto field = fields->Get( i );

            sprintf_s( s_string_buffer, 128, "%s", field->name()->c_str() );
            ImGui::Text( "    %s", s_string_buffer );

            // This is purely empirical. Seems to indicate that this is an enum if != -1.
            int field_index = field->type()->index();
            if ( field_index >= 0 ) {
                const reflection::Enum* enum_ = enums->Get( field_index );
                ImGui::Text( "   Enum  %s", enum_->name()->c_str() );

                static int32_t s_data = 0;
                ImGui::Combo( s_string_buffer, &s_data, s_texture_formats_enum_values, s_texture_formats_enum_data.size() );
            }
            else {
                // 5.2. Add a custom widget based on type
                reflection::BaseType field_base_type = field->type()->base_type();

                switch ( field_base_type ) {
                    case reflection::BaseType::UShort:
                    {
                        static uint16_t s_data;
                        ImGui::InputScalar( s_string_buffer, ImGuiDataType_U16, &s_data );
                        break;
                    }

                    case reflection::BaseType::Float:
                    {
                        static float s_data;
                        ImGui::InputScalar( s_string_buffer, ImGuiDataType_Float, &s_data );
                        break;
                    }
                }
            }

            // 5.3 Search for attributes that can be used for anything.
            auto field_attributes = field->attributes();
            if ( field_attributes ) {
                auto ui = field_attributes->LookupByKey( "ui" );

                if ( ui ) {
                    ImGui::Text( "       UI attribute: %s", ui->value()->c_str() );
                }
            }
        }
    }

    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::NewLine();
}

static void ReflectUIMini( const flatbuffers::TypeTable& type_table, FlatBuffersReflectionTable& reflection_table ) {

    static char s_string_buffer[128];

    ImGui::Separator();
    ImGui::NewLine();
    ImGui::Text( "Flatbuffers UI Mini Reflection" );
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();
    
    for ( uint32_t i = 0; i < type_table.num_elems; ++i ) {
        const flatbuffers::TypeCode& type_code = type_table.type_codes[i];
        ImGui::Text( "%s: %s", type_table.names[i], flatbuffers::ElementaryTypeNames()[type_code.base_type] );

        sprintf_s( s_string_buffer, 128, "%s", type_table.names[i] );

        if ( type_code.sequence_ref == 0 ) {
            if ( type_table.type_refs[type_code.sequence_ref] ) {
                const flatbuffers::TypeTable* enum_type = type_table.type_refs[type_code.sequence_ref]();

                ImGui::Combo( s_string_buffer, (int32_t*)reflection_table.GetData( i ), enum_type->names, enum_type->num_elems );
            }
        }
        else {
            switch ( type_code.base_type ) {

                case flatbuffers::ET_BOOL:
                {
                    ImGui::Checkbox( s_string_buffer, (bool*)reflection_table.GetData( i ) );
                    break;
                }

                case flatbuffers::ET_CHAR:
                {
                    ImGui::InputScalar( s_string_buffer, ImGuiDataType_S8, reflection_table.GetData( i ) );
                    break;
                }

                case flatbuffers::ET_UCHAR:
                {
                    ImGui::InputScalar( s_string_buffer, ImGuiDataType_U8, reflection_table.GetData( i ) );
                    break;
                }

                case flatbuffers::ET_SHORT:
                {
                    ImGui::InputScalar( s_string_buffer, ImGuiDataType_S16, reflection_table.GetData( i ) );
                    break;
                }

                case flatbuffers::ET_USHORT:
                {
                    ImGui::InputScalar( s_string_buffer, ImGuiDataType_U16, reflection_table.GetData( i ) );
                    break;
                }

                case flatbuffers::ET_INT:
                {
                    break;
                }

                case flatbuffers::ET_UINT:
                {
                    break;
                }

                case flatbuffers::ET_FLOAT:
                {
                    ImGui::InputScalar( s_string_buffer, ImGuiDataType_Float, reflection_table.GetData( i ) );
                    break;
                }

                case flatbuffers::ET_LONG:
                {
                    break;
                }

                case flatbuffers::ET_ULONG:
                {
                    break;
                }

                case flatbuffers::ET_DOUBLE:
                {
                    break;
                }

                case flatbuffers::ET_SEQUENCE:
                {
                    break;
                }

                default:
                {
                    assert( false && "Error: type not supported" );
                    break;
                }
            }
        }
    }
}

static void ShowReflectionUI() {    

    ImGui::Begin( "Flatbuffers Reflection UI" );
    using namespace flatbuffers;

    // 1. Flatbuffers full-reflection.
    //    Obtained from a binary fbs file generated from a schema fbs!
    //    flatc.exe -b --schema reflection.fbs RenderDefinitions.fbs
    //
    //    Outputs RenderDefinitions.bfbs (binary fbs)
    ReflectUIFull();

    ImGui::End();

    ImGui::Begin( "Flatbuffers Mini-Reflection UI" );
    // 2. Flatbuffers mini-reflection.
    //    Obtained from a fbs file, it generates helpers in the headers to reflect.
    //    flatc --cpp RenderDefinitions.fbs --reflect-names
    //
    //    Outputs RenderDefinitions_generated.h with all the reflection data.
    const TypeTable* rt_table = rendering::RenderTargetTypeTable();

    static FlatBuffersReflectionTable rt_reflection_table;
    rt_reflection_table.Populate( rt_table );

    ReflectUIMini( *rt_table, rt_reflection_table );

    ImGui::End();
}


int main(int argc, char** argv) {

    if ( SDL_Init( SDL_INIT_EVERYTHING ) != 0 ) {
        printf( "SDL Init error: %s\n", SDL_GetError() );
        return -1;
    }

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, 0 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );

    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode( 0, &current );
    SDL_WindowFlags window_flags = (SDL_WindowFlags)( SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI );
    SDL_Window* window = SDL_CreateWindow( "Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags );
    SDL_GLContext gl_context = SDL_GL_CreateContext( window );
    SDL_GL_SetSwapInterval( 1 ); // Enable vsync

    // Initialize OpenGL loader
    bool err = glewInit() != GLEW_OK;
    if ( err )
    {
        fprintf( stderr, "Failed to initialize OpenGL loader!\n" );
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL( window, gl_context );
    ImGui_ImplOpenGL3_Init( glsl_version );

    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4( 0.45f, 0.55f, 0.60f, 1.00f );

    // Main loop
    bool done = false;
    while ( !done )
    {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) )
        {
            ImGui_ImplSDL2_ProcessEvent( &event );
            if ( event.type == SDL_QUIT )
                done = true;
            if ( event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID( window ) )
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame( window );
        ImGui::NewFrame();

        if ( show_demo_window )
            ImGui::ShowDemoWindow( &show_demo_window );

        ShowReflectionUI();

        // Rendering
        ImGui::Render();
        SDL_GL_MakeCurrent( window, gl_context );
        glViewport( 0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y );
        glClearColor( clear_color.x, clear_color.y, clear_color.z, clear_color.w );
        glClear( GL_COLOR_BUFFER_BIT );
        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
        SDL_GL_SwapWindow( window );
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext( gl_context );
    SDL_DestroyWindow( window );
    SDL_Quit();

    return 0;
}
