"""Build the fading closet body material inside Unreal Editor."""

import unreal


def prepare_closet_presence(tools):
    package = '/Game/StillAwake'
    name = 'M_ClosetPresence'
    if unreal.EditorAssetLibrary.does_asset_exist(f'{package}/{name}'):
        return

    atlas = unreal.load_asset(f'{package}/Art/T_HouseSurfaces')
    function = unreal.load_asset('/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA')
    if not atlas or not function:
        raise RuntimeError('Closet presence material requires the surface atlas and temporal dither function')

    material = tools.create_asset(name, package, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    edit = unreal.MaterialEditingLibrary

    def expression(kind, x, y):
        return edit.create_material_expression(material, kind, x, y)

    def connect(source, output, destination, port):
        if not edit.connect_material_expressions(source, output, destination, port):
            available = edit.get_material_expression_input_names(destination)
            raise RuntimeError(f'Could not wire closet presence input {port!r} from {available}')

    coordinates = expression(unreal.MaterialExpressionTextureCoordinate, -760, 40)
    coordinates.set_editor_property('u_tiling', .23)
    coordinates.set_editor_property('v_tiling', .23)
    origin = expression(unreal.MaterialExpressionConstant2Vector, -760, 190)
    origin.set_editor_property('r', .51)
    origin.set_editor_property('g', .76)
    tile = expression(unreal.MaterialExpressionAdd, -520, 40)
    connect(coordinates, '', tile, 'A')
    connect(origin, '', tile, 'B')

    texture = expression(unreal.MaterialExpressionTextureSample, -280, 40)
    texture.set_editor_property('texture', atlas)
    connect(tile, '', texture, '')
    tint = expression(unreal.MaterialExpressionConstant3Vector, -280, 260)
    tint.set_editor_property('constant', unreal.LinearColor(.038, .027, .025))
    color = expression(unreal.MaterialExpressionMultiply, -20, 40)
    connect(texture, '', color, 'A')
    connect(tint, '', color, 'B')

    opacity = expression(unreal.MaterialExpressionScalarParameter, -530, 520)
    opacity.set_editor_property('parameter_name', 'PresenceOpacity')
    opacity.set_editor_property('default_value', 0.0)
    dither = expression(unreal.MaterialExpressionMaterialFunctionCall, -220, 520)
    if not dither.set_material_function(function):
        raise RuntimeError('Could not attach the temporal dither function')
    connect(opacity, '', dither, 'Alpha Threshold')

    if not edit.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR):
        raise RuntimeError('Could not wire closet body color')
    if not edit.connect_material_property(dither, '', unreal.MaterialProperty.MP_OPACITY_MASK):
        raise RuntimeError('Could not wire closet body opacity')
    edit.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Could not save the closet presence material')
