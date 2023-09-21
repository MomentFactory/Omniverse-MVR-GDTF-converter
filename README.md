# MF.OV.MVR and MF.OV.GDTF

This repository contains 2x extensions. 
Brings support of MVR and GDTF files to Omniverse and USD.

GDTF (General Device Type Format) defines an asset format that collects  information about Audiovisual devices. It is currently centered on lighting fixtures and provide accurate digital twins of lighting devices from 100+ manufacturers.

MVR (My Virtual Rig) is a scene format that can describe an complete rig of lights, using GDTF assets at its core while adding capabilities to define groups, layers, DMX address and more to allow lighting designer to build virtual replicas of their lighting rigs and enforce a single file format from show design to previz to operation. 

This repository contains two separate extensions :

- [MVR extension](./exts/mf.ov.mvr/)
- [GDTF extension](./exts/mf.ov.gdtf/)


# Getting started

- Requires Omniverse Kit >= 105
- Tested in Create 2023.1.1

## From Omniverse exchange

Simply search for `MF GDTF Converter` or `MF MVR Converter` and enable them.

# Convert MVR or GDTF files

Note: to properly work with MVR files, both extension have to be enabled. 

1. In the content tab, browse to the folder where you want to import your `MVR` or `GDTF` files.
2. Click the `+Import` button and select "External Assets (FBX, OBJ...)
3. Choose a `MVR` or `GDTF` file and wait for it to import.
-  MVR import
   - The import result will be stored in a folder with the same name as the imported file in the current content browser directory.
   - if `GDTF` files are referenced, they will be converted to `USD` in a subfolder.
- GDTF import
  - The import result will be stored in a folder with the same name as the imported file in the current content browser directory.

1. To finalize the import, drag the freshly converted `USD` file in your project.

# Sample files 

GDTF 

https://gdtf-share.com/share.php?page=home&manu=Robe+Lighting&fix=Robin+Spiider



# `MVR.USD` USD schema

Note : not every aspect of the MVR specification is currently implemented for USD, as we focused on the ability to retrieve the lighting fixture information. 

1. Under the Root, you'll find `Scope` representing the different `Layers` of the MVR scene.
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

# MVR Raw USD Properties

TODO


TODO 


TODO

# `GDTF.USD` USD Schema

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
- Matrix is in millimeters (applies to the last part, the translation).
- Color is in [CIE 1931 color space](https://en.wikipedia.org/wiki/CIE_1931_color_space) and represent the color of a color gel or similar apparatus and not of the fixture itself.

# Resources

- [MVR and GDTF homepage with Fixture Library](https://gdtf-share.com/)
- [Specifications Github repostory](https://github.com/mvrdevelopment/spec)
