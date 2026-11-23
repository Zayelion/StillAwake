"""Run inside UnrealEditor-Cmd with -run=pythonscript -script=this_file."""
from pathlib import Path
from runpy import run_path

import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
material_path = '/Game/StillAwake/M_House'
if not unreal.EditorAssetLibrary.does_asset_exist(material_path):
    material = tools.create_asset('M_House','/Game/StillAwake',unreal.Material,unreal.MaterialFactoryNew())
    tint = unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-300,0)
    tint.set_editor_property('parameter_name','Tint')
    tint.set_editor_property('default_value',unreal.LinearColor(.5,.45,.35,1))
    rough = unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant,-300,180)
    rough.set_editor_property('r',.8)
    unreal.MaterialEditingLibrary.connect_material_property(tint,'',unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Could not save M_House material')


def prepare_texture(name, filename, ui=False):
    folder = '/Game/StillAwake/Art'
    path = f'{folder}/{name}'
    content = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
    source = content / 'Art' / filename
    package = content / 'StillAwake' / 'Art' / f'{name}.uasset'
    if not source.is_file():
        raise RuntimeError(f'Missing source artwork: {source}')
    if package.is_file() and package.stat().st_mtime_ns >= source.stat().st_mtime_ns:
        texture = unreal.EditorAssetLibrary.load_asset(path)
        if texture:
            return texture

    task = unreal.AssetImportTask()
    task.set_editor_properties({
        'filename': str(source),
        'destination_path': folder,
        'destination_name': name,
        'automated': True,
        'replace_existing': True,
        'save': False,
        'factory': unreal.TextureFactory(),
    })
    tools.import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(path)
    if not texture:
        raise RuntimeError(f'Could not import {source} as {path}')
    texture.set_editor_properties({
        'address_x': unreal.TextureAddress.TA_CLAMP,
        'address_y': unreal.TextureAddress.TA_CLAMP,
        'never_stream': True,
    })
    if ui:
        texture.set_editor_properties({
            'lod_group': unreal.TextureGroup.TEXTUREGROUP_UI,
            'compression_settings': unreal.TextureCompressionSettings.TC_EDITOR_ICON,
            'mip_gen_settings': unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,
        })
    if not ui:
        texture.set_editor_properties({
            'power_of_two_mode': unreal.TexturePowerOfTwoSetting.RESIZE_TO_SPECIFIC_RESOLUTION,
            'resize_during_build_x': 1024,
            'resize_during_build_y': 1024,
            'mip_gen_settings': unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE,
        })
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
        raise RuntimeError(f'Could not save {path}')
    return texture


def prepare_textured_material(atlas):
    path = '/Game/StillAwake/M_HouseTextured'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return

    edit = unreal.MaterialEditingLibrary
    material = tools.create_asset('M_HouseTextured', '/Game/StillAwake', unreal.Material, unreal.MaterialFactoryNew())
    edit.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)

    def expression(kind, x, y):
        return edit.create_material_expression(material, kind, x, y)

    def parameter(kind, name, default, y):
        node = expression(kind, -740, y)
        node.set_editor_property('parameter_name', name)
        node.set_editor_property('default_value', default)
        return node

    def connect(source, target, name):
        if not edit.connect_material_expressions(source, '', target, name):
            raise RuntimeError(f'Could not connect M_HouseTextured input {name}')

    def output(source, prop):
        if not edit.connect_material_property(source, '', prop):
            raise RuntimeError(f'Could not connect M_HouseTextured property {prop}')

    tint = parameter(unreal.MaterialExpressionVectorParameter, 'Tint', unreal.LinearColor(.5, .45, .35, 1), 0)
    scale = parameter(unreal.MaterialExpressionVectorParameter, 'GrainScale', unreal.LinearColor(.025, .025, 0, 0), 160)
    detail = parameter(unreal.MaterialExpressionScalarParameter, 'SurfaceDetail', .7, 320)
    roughness = parameter(unreal.MaterialExpressionScalarParameter, 'Roughness', .74, 480)
    metal = parameter(unreal.MaterialExpressionScalarParameter, 'Metallic', 0.0, 640)
    emission = parameter(unreal.MaterialExpressionScalarParameter, 'Emission', 0.0, 800)
    tile = parameter(unreal.MaterialExpressionVectorParameter, 'AtlasTile', unreal.LinearColor(0, 0, .778, 0), -200)
    position = expression(unreal.MaterialExpressionWorldPosition, -740, -360)
    normal = expression(unreal.MaterialExpressionPixelNormalWS, -740, -520)
    texture = expression(unreal.MaterialExpressionTextureObjectParameter, -740, -680)
    texture.set_editor_properties({'parameter_name': 'SurfaceAtlas', 'texture': atlas})
    surface = expression(unreal.MaterialExpressionCustom, -360, 0)
    surface.set_editor_property('description', 'World-aligned surface atlas')
    surface.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs = []
    for name in ('Position', 'Normal', 'Color', 'Scale', 'Strength', 'Tile', 'Atlas'):
        port = unreal.CustomInput()
        port.set_editor_property('input_name', name)
        inputs.append(port)
    surface.set_editor_property('inputs', inputs)
    surface.set_editor_property('code', """
struct SurfaceProjection
{
    float Sample(Texture2D Image, SamplerState ImageSampler, float2 Point, float3 Cell)
    {
        // Mirror at tile edges and keep bilinear taps inside the selected atlas cell.
        float2 Mirrored = 1 - abs(frac(Point * .5) * 2 - 1);
        float2 UV = (Cell.xy + lerp(.08, .92, Mirrored)) * .25;
        float Footprint = max(length(ddx(Point)), length(ddy(Point))) * (256 * .84);
        float Mip = log2(max(1, Footprint));
        float3 RGB = Image.SampleLevel(ImageSampler, UV, min(Mip, 4)).rgb;
        float Luma = sqrt(dot(RGB, float3(.2126, .7152, .0722)));
        return (Luma - Cell.z) * saturate(5.5 - Mip);
    }
};
SurfaceProjection Projection;
float3 Weights = pow(abs(Normal), 8);
Weights /= max(dot(Weights, 1), .001);
float3 Grain = float3(
    Projection.Sample(Atlas, AtlasSampler, Position.yz * Scale.xy, Tile.xyz),
    Projection.Sample(Atlas, AtlasSampler, Position.xz * Scale.xy, Tile.xyz),
    Projection.Sample(Atlas, AtlasSampler, Position.yx * Scale.xy, Tile.xyz));
return saturate(Color.rgb * (1 + Strength * clamp(3.4 * dot(Grain, Weights), -.74, .74)));
""")
    for source, name in ((position, 'Position'), (normal, 'Normal'), (tint, 'Color'), (scale, 'Scale'),
                         (detail, 'Strength'), (tile, 'Tile'), (texture, 'Atlas')):
        connect(source, surface, name)
    glow = expression(unreal.MaterialExpressionMultiply, -360, 800)
    connect(tint, glow, 'A')
    connect(emission, glow, 'B')
    output(surface, unreal.MaterialProperty.MP_BASE_COLOR)
    output(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
    output(metal, unreal.MaterialProperty.MP_METALLIC)
    output(glow, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Could not save M_HouseTextured material')


def prepare_map():
    map_path = '/Game/StillAwake/Maps/House'
    if unreal.EditorAssetLibrary.does_asset_exist(map_path):
        return
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(map_path):
        raise RuntimeError('Could not create House map')
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-145, 0, 90), unreal.Rotator(0, 0, 0))
    start.set_actor_label('Porch start')
    if not levels.save_current_level():
        raise RuntimeError('Could not save new House map')


if __name__ == '__main__':
    prepare_textured_material(prepare_texture('T_HouseSurfaces', 'HouseSurfaces.png'))
    prepare_texture('T_PeripheralHorrors', 'PeripheralHorrors.png', ui=True)
    prepare_closet_presence = run_path(str(Path(__file__).with_name('PrepareClosetPresence.py')))['prepare_closet_presence']
    prepare_closet_presence(tools)
    prepare_map()
    unreal.log('STILL AWAKE CONTENT READY v0.3: House surface atlas, peripheral sprites, and closet material.')
