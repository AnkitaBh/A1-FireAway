# ssh team-a1@team-a1.wifi.local.cmu.edu
# CMU 18500 Capstone Team A1: FIREAWAY
# Web Application

# ssh team-a1@team-a1.wifi.local.cmu.edu
# CMU 18500 Capstone Team A1: FIREAWAY

# @author: Ankita Bhanjois
# @summary: Web Application to display what is happening in our nodes in our wireless sensor network

from django.shortcuts import render
from django.http import HttpResponse
from django.http import JsonResponse
from django.contrib import messages
import folium, json, os

# coordinates for nodes from geojson.io
map_coords = [37.762135068106375, -119.5743007660738]
gateway_coords = [37.73759006636105, -119.57600026864158]
node1_coords = [37.751379013728624, -119.59598664099607]
node2_coords = [37.757950419688925, -119.57428437820917]
node3_coords = [37.75101391850616, -119.5565839511132]
node4_coords = [37.76610301940818, -119.60106589398869]
node5_coords = [37.77615107217039, -119.57419067235384]
node6_coords = [37.76997211495174, -119.55054400404586]
node7_coords = [37.785845245544465, -119.59210298473181]
node8_coords = [37.78754164874421, -119.55918940322921]

# connection links
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
node4_node7 = [node4_coords, node7_coords]

node5_node6 = [node5_coords, node6_coords]
node5_node7 = [node5_coords, node7_coords]
node5_node8 = [node5_coords, node8_coords]

node6_node8 = [node6_coords, node8_coords]

node7_node8 = [node7_coords, node8_coords]

# create the views here.

# web page for the 'about' page
def index(request):
    return render(request, 'index.html')

# web page for the 'how to use' page
def user(request):
    return render(request, 'user.html')

def readjsonfile():
    # myfile = open('/home/team-a1/Wildfire_WSN/sample.json')
    myfile = open('/Users/ankitabhanjois/Desktop/fireaway/sample.json')
    data = json.load(myfile)
    fire_node = data['firenode']
    dead_nodes = data['deadnodes']
    stp_links = data['activelinks']
    myfile.close()
    return fire_node, dead_nodes, stp_links

def make_conns(m):
    stp_conn_color = "lightgrey"
    line_weight = 1.5

    folium.PolyLine(gw_node1, color = stp_conn_color, weight=line_weight).add_to(m)
    folium.PolyLine(gw_node2, color = stp_conn_color, weight=line_weight).add_to(m)
    folium.PolyLine(gw_node3, color = stp_conn_color, weight=line_weight).add_to(m)
    
    folium.PolyLine(node1_node2, color = stp_conn_color, weight=line_weight).add_to(m)
    folium.PolyLine(node1_node4, color = stp_conn_color, weight=line_weight).add_to(m)
    
    folium.PolyLine(node2_node3, color = stp_conn_color, weight=line_weight).add_to(m)
    folium.PolyLine(node2_node4, color = stp_conn_color, weight=line_weight).add_to(m)
    folium.PolyLine(node2_node5, color = stp_conn_color, weight=line_weight).add_to(m)
    folium.PolyLine(node2_node6, color = stp_conn_color, weight=line_weight).add_to(m)
    
    folium.PolyLine(node3_node6, color = stp_conn_color, weight=line_weight).add_to(m)
    
    folium.PolyLine(node4_node5, color = stp_conn_color, weight=line_weight).add_to(m)

    folium.PolyLine(node5_node6, color = stp_conn_color, weight=line_weight).add_to(m)
    
    # comment out these lines if converting to MESH-7
    folium.PolyLine(node4_node7, color = stp_conn_color, weight=line_weight).add_to(m)
    folium.PolyLine(node5_node7, color = stp_conn_color, weight=line_weight).add_to(m)
    folium.PolyLine(node5_node8, color = stp_conn_color, weight=line_weight).add_to(m)

    folium.PolyLine(node7_node8, color = stp_conn_color, weight=line_weight).add_to(m)
    folium.PolyLine(node6_node8, color = stp_conn_color, weight=line_weight).add_to(m)
    return

def parse_links(stp_links, m):
    used_link_color = "black"
    line_weight = 1.5

    if 1 in stp_links['node0']:
        folium.PolyLine(gw_node1, color = used_link_color, weight=line_weight).add_to(m)
    if 2 in stp_links['node0']:
        folium.PolyLine(gw_node2, color = used_link_color, weight=line_weight).add_to(m)
    if 3 in stp_links['node0']:
        folium.PolyLine(gw_node3, color = used_link_color, weight=line_weight).add_to(m)

    if 2 in stp_links['node1']:
        folium.PolyLine(node1_node2, color = used_link_color, weight=line_weight).add_to(m)
    if 4 in stp_links['node1']:
        folium.PolyLine(node1_node4, color = used_link_color, weight=line_weight).add_to(m)

    if 3 in stp_links['node2']:
        folium.PolyLine(node2_node3, color = used_link_color, weight=line_weight).add_to(m)
    if 4 in stp_links['node2']:
        folium.PolyLine(node2_node4, color = used_link_color, weight=line_weight).add_to(m)
    if 5 in stp_links['node2']:
        folium.PolyLine(node2_node5, color = used_link_color, weight=line_weight).add_to(m)
    if 6 in stp_links['node2']:
        folium.PolyLine(node2_node6, color = used_link_color, weight=line_weight).add_to(m)

    if 6 in stp_links['node3']:
        folium.PolyLine(node3_node6, color = used_link_color, weight=line_weight).add_to(m)
    
    if 5 in stp_links['node4']:
        folium.PolyLine(node4_node5, color = used_link_color, weight=line_weight).add_to(m)
    if 7 in stp_links['node4']:
        folium.PolyLine(node4_node7, color = used_link_color, weight=line_weight).add_to(m)
    
    if 6 in stp_links['node5']:
        folium.PolyLine(node5_node6, color = used_link_color, weight=line_weight).add_to(m)
    if 7 in stp_links['node5']:
        folium.PolyLine(node5_node7, color = used_link_color, weight=line_weight).add_to(m)
    if 8 in stp_links['node5']:
        folium.PolyLine(node5_node8, color = used_link_color, weight=line_weight).add_to(m)

    if 8 in stp_links['node6']:
        folium.PolyLine(node6_node8, color = used_link_color, weight=line_weight).add_to(m)
    
    if 8 in stp_links['node7']:
        folium.PolyLine(node7_node8, color = used_link_color, weight=line_weight).add_to(m)
    
    return


def make_dead_conns(dead_nodes, m):
    # TODO: if node is dead, make its links lightgrey
    dead_conn_color = "lightgrey"
    line_weight = 1.5

    # THE GATEWAY WILL NEVER DIE

    if 1 in dead_nodes:
        folium.PolyLine(gw_node1, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node1_node2, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node1_node4, color = dead_conn_color, weight=line_weight).add_to(m)

    if 2 in dead_nodes:
        folium.PolyLine(gw_node2, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node1_node2, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node2_node3, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node2_node4, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node2_node5, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node2_node6, color = dead_conn_color, weight=line_weight).add_to(m)

    if 3 in dead_nodes:
        folium.PolyLine(gw_node3, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node2_node3, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node3_node6, color = dead_conn_color, weight=line_weight).add_to(m)

    if 4 in dead_nodes:
        folium.PolyLine(node1_node4, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node2_node4, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node4_node5, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node4_node7, color = dead_conn_color, weight=line_weight).add_to(m)

    if 5 in dead_nodes:
        folium.PolyLine(node2_node5, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node4_node5, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node5_node6, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node5_node7, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node5_node8, color = dead_conn_color, weight=line_weight).add_to(m)

    if 6 in dead_nodes:
        folium.PolyLine(node2_node6, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node3_node6, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node5_node6, color = dead_conn_color, weight=line_weight).add_to(m)
        # folium.PolyLine(node6_node7, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node6_node8, color = dead_conn_color, weight=line_weight).add_to(m)

    if 7 in dead_nodes:
        folium.PolyLine(node4_node7, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node5_node7, color = dead_conn_color, weight=line_weight).add_to(m)
        # folium.PolyLine(node6_node7, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node7_node8, color = dead_conn_color, weight=line_weight).add_to(m)

    if 8 in dead_nodes:
        folium.PolyLine(node5_node8, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node6_node8, color = dead_conn_color, weight=line_weight).add_to(m)
        folium.PolyLine(node7_node8, color = dead_conn_color, weight=line_weight).add_to(m)

    return


def make_nodes(fire_node, dead_nodes, m):
    # gateway node (node 0)
    folium.Marker(location = gateway_coords, icon = folium.Icon(color='darkblue', icon='home'), tooltip="You are Here.").add_to(m)

    print(fire_node, dead_nodes)
    # node 1
    if (1 not in fire_node) and (1 not in dead_nodes):
        node = folium.Marker(location=node1_coords, icon = folium.Icon(color='blue'), tooltip="Node 1 " + str(node1_coords)).add_to(m)
    elif (1 in dead_nodes):
        node = folium.Marker(location=node1_coords, icon = folium.Icon(color='lightgray',icon="ban", prefix='fa'), tooltip="Node 1 " + str(node1_coords)).add_to(m)
    else:
        node = folium.Marker(location=node1_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 1 " + str(node1_coords), popup=folium.Popup('FIRE AT NODE 1',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    # node 2
    if (2 not in fire_node) and (2 not in dead_nodes):
        folium.Marker(location=node2_coords, icon = folium.Icon(color='blue'), tooltip="Node 2 " + str(node2_coords)).add_to(m)
    elif (2 in dead_nodes):
        folium.Marker(location=node2_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 2 " + str(node2_coords)).add_to(m)
    else:
        folium.Marker(location=node2_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 2 " + str(node2_coords), popup=folium.Popup('FIRE AT NODE 2',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    # node 3
    if (3 not in fire_node) and (3 not in dead_nodes):
        folium.Marker(location=node3_coords, icon = folium.Icon(color='blue'), tooltip="Node 3 " + str(node3_coords)).add_to(m)
    elif (3 in dead_nodes):
        folium.Marker(location=node3_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 3 " + str(node3_coords)).add_to(m)
    else: 
        folium.Marker(location=node3_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 3 " + str(node3_coords), popup=folium.Popup('FIRE AT NODE 3',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    # node 4
    if (4 not in fire_node) and (4 not in dead_nodes):
        folium.Marker(location=node4_coords, icon = folium.Icon(color='blue'), tooltip="Node 4 " + str(node4_coords)).add_to(m)
    elif (4 in dead_nodes):
        folium.Marker(location=node4_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 4 " + str(node4_coords)).add_to(m)
    else:
        folium.Marker(location=node4_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 4 " + str(node4_coords), popup=folium.Popup('FIRE AT NODE 4',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    # node 5
    if (5 not in fire_node) and (5 not in dead_nodes):
        folium.Marker(location=node5_coords, icon = folium.Icon(color='blue'), tooltip="Node 5 " + str(node5_coords)).add_to(m)
    elif (5 in dead_nodes):
        folium.Marker(location=node5_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 5 " + str(node5_coords)).add_to(m)
    else:
        folium.Marker(location=node5_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 5 " + str(node5_coords), popup=folium.Popup('FIRE AT NODE 5',
                                 show=True, min_width=90, max_width=90)).add_to(m)    
    # node 6
    if (6 not in fire_node) and (6 not in dead_nodes):
        folium.Marker(location=node6_coords, icon = folium.Icon(color='blue'), tooltip="Node 6 " + str(node6_coords)).add_to(m)
    elif (6 in dead_nodes):
        folium.Marker(location=node6_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 6 " + str(node6_coords)).add_to(m)
    else:
        folium.Marker(location=node6_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 6 " + str(node6_coords), popup=folium.Popup('FIRE AT NODE 6',
                                 show=True, min_width=90, max_width=90)).add_to(m)

    # comment out next two nodes if doing MESH-7
    # node 7
    if (7 not in fire_node) and (7 not in dead_nodes):
        folium.Marker(location=node7_coords, icon = folium.Icon(color='blue'), tooltip="Node 7 " + str(node7_coords)).add_to(m)
    elif (7 in dead_nodes):
        folium.Marker(location=node7_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 7 " + str(node7_coords)).add_to(m)
    else:
        folium.Marker(location=node7_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 7 " + str(node7_coords), popup=folium.Popup('FIRE AT NODE 7',
                                 show=True, min_width=90, max_width=90)).add_to(m)

    # node 8
    if (8 not in fire_node) and (8 not in dead_nodes):
        folium.Marker(location=node8_coords, icon = folium.Icon(color='blue'), tooltip="Node 8 " + str(node8_coords)).add_to(m)
    elif (8 in dead_nodes):
        folium.Marker(location=node8_coords, icon = folium.Icon(color='lightgray', icon="ban", prefix='fa'), tooltip="Node 8 " + str(node8_coords)).add_to(m)
    else:
        folium.Marker(location=node8_coords, icon =folium.Icon(color="red",icon="fire", prefix='glyphicon'), tooltip="Node 8 " + str(node8_coords), popup=folium.Popup('FIRE AT NODE 8',
                                 show=True, min_width=90, max_width=90)).add_to(m)
    return

def map(request):
    # specify the longitude and latitude for the map
    # zoom into the map enough to see the map
    
    # creation of the map centered at specified coordinates
    # for our project: we are centered at the Visitors Center at Yosemite National Parks
    fire_node, dead_nodes, stp_links = readjsonfile()
    m = folium.Map(location = map_coords, zoom_start = 13)

    make_conns(m) 
    make_nodes(fire_node, dead_nodes, m)
    parse_links(stp_links, m)
    make_dead_conns(dead_nodes, m)
    
    m = m._repr_html_()

    context = {
        'm':m
    }

    return render(request, 'map.html', context)
