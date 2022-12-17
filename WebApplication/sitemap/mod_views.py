# ssh team-a1@team-a1.wifi.local.cmu.edu
# CMU 18500 Capstone Team A1: FIREAWAY

# @author: Ankita Bhanjois
# @summary: Web Application to display what is happening in our nodes in our wireless sensor network

from django.shortcuts import render
from django.http import HttpResponse
from django.http import JsonResponse
from django.contrib import messages
import folium, json, os

# TODO: find the appropriate coordinates from geojson.io -- FIRST
map_coords = [37.77832273127929, -119.57130081408462]
gateway_coords = [37.739962027435055, -119.57356273302027]
node1_coords = [37.76559921845947, -119.60612169502213]
node2_coords = [37.78065946505164, -119.57162931352237]
node3_coords = [37.767416997164815, -119.54009342129916]
node4_coords = [37.801167624353155, -119.61039217978414]
node5_coords = [37.820372635373914, -119.57064381634157]
node6_coords = [37.80376318801301, -119.53812242807062]

# all the links connecting the nodes
gw_node1 = [gateway_coords, node1_coords]
gw_node2 = [gateway_coords, node2_coords]
gw_node3 = [gateway_coords, node3_coords]

node1_node2 = [node1_coords, node2_coords]
node1_node4 = [node1_coords, node4_coords]

node2_node3 = [node2_coords, node3_coords]
node2_node4 = [node2_coords, node4_coords]
node2_node5 = [node2_coords, node5_coords]
node2_node6 = [node2_coords, node6_coords]

node3_node6 = [node3_coords, node6_coords]
node4_node5 = [node4_coords, node5_coords]
node5_node6 = [node5_coords, node6_coords]

# create the views here.

# web page for the 'about' page
def index(request):
    return render(request, 'index.html')

# web page for the 'how to use' page
def user(request):
    return render(request, 'user.html')

def readjsonfile():
    # myfile = open('/home/team-a1/Wildfire_WSN/sample.json')
    myfile = open('/Users/ankitabhanjois/Desktop/fireaway/stp.json')
    data = json.load(myfile)
    fire_node = data['firenode']
    dead_nodes = data['deadnodes']
    stp_links = data['activelinks']
    myfile.close()
    return fire_node, dead_nodes, stp_links

def parse_links(stp_links, m):
    # TODO: this is where the black lines will go
    return

def make_conns(m):
    # TODO: make all lines default lightgrey?
    return

def make_dead_conns(m):
    # TODO: if node is dead, make its links lightgrey
    return

def make_nodes(fire_node, dead_nodes, m):
    # TODO: create the markers and map -- DO THIS FIRST
    # gateway node (node 0)
    folium.Marker(location = gateway_coords, icon = folium.Icon(color='darkblue', icon='home'), tooltip="You are Here.").add_to(m)

    # node 1
    if (1 not in fire_node) and (1 not in dead_nodes):
        node = folium.Marker(location=node1_coords, icon = folium.Icon(color='blue'), tooltip="Node 1 [37.7447742193898, -119.58028694168499]").add_to(m)
    elif (1 in dead_nodes):
        node = folium.Marker(location=node1_coords, icon = folium.Icon(color='lightgray',icon="ban", prefix='fa'), tooltip="Node 1 [37.7447742193898, -119.58028694168499]").add_to(m)
    else:
        node = folium.Marker(location=node1_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 1 [37.7447742193898, -119.58028694168499]", popup=folium.Popup('FIRE AT NODE 1',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    # node 2
    if (2 not in fire_node) and (2 not in dead_nodes):
        folium.Marker(location=node2_coords, icon = folium.Icon(color='blue'), tooltip="Node 2 [37.745825213870845, -119.56942704926747]").add_to(m)
    elif (2 in dead_nodes):
        folium.Marker(location=node2_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 2 [37.745825213870845, -119.56942704926747]").add_to(m)
    else:
        folium.Marker(location=node2_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 2 [37.745825213870845, -119.56942704926747]", popup=folium.Popup('FIRE AT NODE 2',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    # node 3
    if (3 not in fire_node) and (3 not in dead_nodes):
        folium.Marker(location=node3_coords, icon = folium.Icon(color='blue'), tooltip="Node 3 [37.74409087465216, -119.55948761341482]").add_to(m)
    elif (3 in dead_nodes):
        folium.Marker(location=node3_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 3 [37.74409087465216, -119.55948761341482]").add_to(m)
    else: 
        folium.Marker(location=node3_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 3 [37.74409087465216, -119.55948761341482]", popup=folium.Popup('FIRE AT NODE 3',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    # node 4
    if (4 not in fire_node) and (4 not in dead_nodes):
        folium.Marker(location=node4_coords, icon = folium.Icon(color='blue'), tooltip="Node 4 [37.751567256888464, -119.58345323479386]").add_to(m)
    elif (4 in dead_nodes):
        folium.Marker(location=node4_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 4 [37.751567256888464, -119.58345323479386]").add_to(m)
    else:
        folium.Marker(location=node4_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 4 [37.751567256888464, -119.58345323479386]", popup=folium.Popup('FIRE AT NODE 4',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    # node 5
    if (5 not in fire_node) and (5 not in dead_nodes):
        folium.Marker(location=node5_coords, icon = folium.Icon(color='blue'), tooltip="Node 5 [37.75271336934124, -119.56987316697862]").add_to(m)
    elif (5 in dead_nodes):
        folium.Marker(location=node5_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 5 [37.75271336934124, -119.56987316697862]").add_to(m)
    else:
        folium.Marker(location=node5_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 5 [37.75271336934124, -119.56987316697862]", popup=folium.Popup('FIRE AT NODE 5',
                                 show=True, min_width=90, max_width=90)).add_to(m)    
    # node 6
    if (6 not in fire_node) and (6 not in dead_nodes):
        folium.Marker(location=node6_coords, icon = folium.Icon(color='blue'), tooltip="Node 6 [37.749396742983876, -119.55752237921965]").add_to(m)
    elif (6 in dead_nodes):
        folium.Marker(location=node6_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 6 [37.749396742983876, -119.55752237921965]").add_to(m)
    else:
        folium.Marker(location=node6_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 6 [37.749396742983876, -119.55752237921965]", popup=folium.Popup('FIRE AT NODE 6',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    return

def map(request):
    # specify the longitude and latitude for the map
    # zoom into the map enough to see the map
    
    # creation of the map centered at specified coordinates
    # for our project: we are centered at the Visitors Center at Yosemite National Parks
    fire_node, dead_nodes, stp_links = readjsonfile()
    m = folium.Map(location = map_coords, zoom_start = 14)

    make_conns(m) 
    make_nodes(fire_node, dead_nodes, m)
    make_dead_conns(dead_nodes, m)
    parse_links(stp_links, m)
    
    m = m._repr_html_()

    context = {
        'm':m
    }

    return render(request, 'map.html', context)