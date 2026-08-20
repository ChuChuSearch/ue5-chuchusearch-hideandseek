import unreal

system = unreal.load_asset('/Game/ToonProjectile/NS_Projectile_01')
unreal.log('SYSTEM_CLASS=' + system.get_class().get_name())
for name in ('emitter_handles', 'emitter_handles_internal', 'exposed_parameters'):
    try:
        value = system.get_editor_property(name)
        unreal.log('PROP {} = {}'.format(name, value))
    except Exception as exc:
        unreal.log_warning('PROP {} unavailable: {}'.format(name, exc))
unreal.log('ATTRS=' + ','.join(a for a in dir(system) if 'emitter' in a.lower()))
