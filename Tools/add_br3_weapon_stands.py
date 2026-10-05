"""One-time additive editor migration. Never recreate maps or retune existing assets."""
import unreal

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
definition = unreal.load_asset('/Game/FPSDemo/Weapons/Definitions/DA_BR3')
assert definition and str(definition.get_editor_property('weapon_id')) == 'Weapon.BR3'
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
dark = unreal.load_asset('/Game/FPSDemo/MapMaterials/M_Map_Dark')
gold = unreal.load_asset('/Game/FPSDemo/MapMaterials/M_Map_Gold')
assert cube and dark and gold

# Add in the gap between existing racks, away from the central combat lane.
placements = {
    'LV_FPSFoundry': [(-2280, -840, 0), (2280, -840, 180)],
    'LV_FPSTraining': [(-2650, -700, 0)],
}
for name, positions in placements.items():
    path = '/Game/FPSDemo/Maps/' + name
    world = unreal.EditorLoadingAndSavingUtils.load_map(path)
    assert world, path
    before = {a.get_path_name() for a in actors.get_all_level_actors()}
    labels = {a.get_actor_label() for a in actors.get_all_level_actors()}
    for index, (x, y, yaw) in enumerate(positions):
        label = 'WeaponStand_BR3_' + str(index)
        if label in labels:
            continue  # Preserve subsequent manual editing.
        stand = actors.spawn_actor_from_class(unreal.FPSWeaponStand, unreal.Vector(x, y, 75),
                                              unreal.Rotator(pitch=0, yaw=yaw, roll=0))
        stand.set_actor_label(label)
        stand.set_editor_property('definition', definition)
        stand.set_editor_property('display_order', 6)
        for suffix, z, size, material, collision in (
                ('Plinth', 18, (115, 100, 36), dark, 'BlockAll'),
                ('Stripe', 37, (110, 90, 2), gold, 'NoCollision')):
            actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z))
            actor.set_actor_label(label + '_' + suffix)
            actor.static_mesh_component.set_static_mesh(cube)
            actor.static_mesh_component.set_material(0, material)
            actor.static_mesh_component.set_collision_profile_name(collision)
            actor.set_actor_scale3d(unreal.Vector(*(v / 100 for v in size)))
    after = {a.get_path_name() for a in actors.get_all_level_actors()}
    assert before <= after, 'Existing actors removed'
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, path)
    unreal.log('FPS_BR3_MAP PASS ' + name + ' added=' + str(len(after) - len(before)))
