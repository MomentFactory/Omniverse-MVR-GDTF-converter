# MF.OV.MVR and MF.OV.GDTF

Brings support of MVR and GDTF files to Omniverse and USD.

GDTF (General Device Type Format) defines an asset format that collects multiple information about Audiovisual devices. It is currently centered on lighting fixtures.

MVR (My Virtual Rig) is a scene description containing GDTF assets, layering them in space, associating them, DMX address them to allow lighting designer to build virtual replicas of their lighting rigs.

This repository contains two different extensions :

- [MVR extension](./exts/mf.ov.mvr/)
- [GDTF extension](./exts/mf.ov.gdtf/)


# Getting started

- Requires Omniverse Kit >= 104.1
- Tested in Create 2022.3.3

## Enable the extension from command line

```
> link_app.bat --app create
> app\omni.create.bat --ext-folder exts --enable mf.ov.mvr
```
## From Omniverse exchange

Simply search for `MF GDTF Converter` or `MF MVR Converter` and enable them.

# Convert MVR or GDTF files

1. In the content tab, browse to the folder where you want to import your `MVR` or `GDTF` files.
2. Click the `+Import` button and select "External Assets (FBX, OBJ...)
3. Choose a `MVR` or `GDTF` file and wait for it to import.
-  MVR import
   - The import result will be stored in a `📁MVR` folder in the current content browser directory.
   - if `GDTF` files are referenced, they will be converted to `USD` in the `📁MVR/gdtf` folder.
- GDTF import
  - The import result will be stored in a `📁GDTF` folder in the current content browser directory.

4. To finalize the import, drag the freshly converted `USD` file in your project.

# `MVR.USD` file structure

1. Under the Root, you'll find `Scope` representing the different layers of the MVR scene.
2. Inside them you'll find each GDTF Fixture represented by an `Xform` pointing to an USD payload.
3. `Xform` are named using their names and their uuid to ensure unique naming.
4.  `Xform` also have custom properties (see Raw USD Properties) using the following convention: `mf:mvr:property`.


```
Root/
└─📁MVR-Layer1 (Scope)
|  ├─💠Make_Model_UID1 (Xform with payload)
|  └─💠Make_Model_UID2 (Xform with payload)
└──📁MVR-Layer1 (Scope)
   └─💠Make_Model_UID1 (Xform with payload)
   └─💠Make_Model_UID2 (Xform with payload)
```

# `GDTF.USD` file structure

GDTF can have multiple structure type, but here is a typical example for a moving light.

```
Root/
└─💠 Base (Xform)
   ├─💠model (Xform)
   │  └─🧊 mesh (Mesh)
   ├─💠Yoke (Xform)
   │  ├─💠model (Xform)
   │  │  └─🧊 mesh (Mesh)
   |  └──💠Head (Xform)
   │      └─💠model (Xform)
   │         └─🧊 mesh (Mesh)
   └─📁Looks (Scope)
```

# Notes

Example of a fixture defined in a MVR file (contains some, but not all properties):
```xml
<Fixture name="Sharpy" uuid="C63B1F8D-6DAD-438C-9228-E33C6EF2947E">
    <Matrix>{-1.000000,0.000000,0.000000}{0.000000,-1.000000,-0.000000}{0.000000,0.000000,1.000000}{-766.333333,4572.000000,7620.000000}</Matrix>
    <GDTFSpec>Clay Paky@Sharpy [Bulb=MSD Platinum 5R 189W].gdtf</GDTFSpec>
    <GDTFMode>Vect</GDTFMode>
    <Addresses>
        <Address break="0">21</Address>
    </Addresses>
    <FixtureID>102</FixtureID>
    <UnitNumber>0</UnitNumber>
    <FixtureTypeId>0</FixtureTypeId>
    <CustomId>0</CustomId>
    <Color>0.312712,0.329008,100.000000</Color>
    <CastShadow>false</CastShadow>
    <Mappings/>
</Fixture>
```

Some notes on the properties:
- Matrix is in milimeters (applies to the last part, the translation).
- Color is in [CIE 1931 color space](https://en.wikipedia.org/wiki/CIE_1931_color_space) and represent the color of a color gel or similar apparatus and not of the fixture itself.

# Resources

- [MVR and GDTF homepage with Fixture Library](https://gdtf-share.com/)
- [Specifications Github repostory](https://github.com/mvrdevelopment/spec)
